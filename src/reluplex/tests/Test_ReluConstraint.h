/*********************                                                        */
/*! \file Test_ReluConstraint.h
** \verbatim
** Top contributors (to current version):
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#include <cxxtest/TestSuite.h>

#include "FreshVariables.h"
#include "MockTableau.h"
#include "PiecewiseLinearCaseSplit.h"
#include "ReluConstraint.h"
#include "ReluplexError.h"

#include <cfloat>
#include <string.h>

class MockForReluConstraint
{
public:
};

class ReluConstraintTestSuite : public CxxTest::TestSuite
{
public:
    MockForReluConstraint *mock;

    void setUp()
    {
        TS_ASSERT( mock = new MockForReluConstraint );
    }

    void tearDown()
    {
        TS_ASSERT_THROWS_NOTHING( delete mock );
    }

    void test_relu_constraint()
    {
        unsigned b = 1;
        unsigned f = 4;

        ReluConstraint relu( b, f );

        List<unsigned> participatingVariables;
        TS_ASSERT_THROWS_NOTHING( participatingVariables = relu.getParticiatingVariables() );
        TS_ASSERT_EQUALS( participatingVariables.size(), 2U );
        auto it = participatingVariables.begin();
        TS_ASSERT_EQUALS( *it, b );
        ++it;
        TS_ASSERT_EQUALS( *it, f );

        TS_ASSERT( relu.participatingVariable( b ) );
        TS_ASSERT( relu.participatingVariable( f ) );
        TS_ASSERT( !relu.participatingVariable( 2 ) );
        TS_ASSERT( !relu.participatingVariable( 0 ) );
        TS_ASSERT( !relu.participatingVariable( 5 ) );

        TS_ASSERT_THROWS_EQUALS( relu.satisfied(),
                                 const ReluplexError &e,
                                 e.getCode(),
                                 ReluplexError::PARTICIPATING_VARIABLES_ABSENT );

        relu.notifyVariableValue( b, 1 );
        relu.notifyVariableValue( f, 1 );

        TS_ASSERT( relu.satisfied() );

        relu.notifyVariableValue( f, 2 );

        TS_ASSERT( !relu.satisfied() );

        relu.notifyVariableValue( b, 2 );

        TS_ASSERT( relu.satisfied() );

        relu.notifyVariableValue( b, -2 );

        TS_ASSERT( !relu.satisfied() );

        relu.notifyVariableValue( f, 0 );

        TS_ASSERT( relu.satisfied() );

        relu.notifyVariableValue( b, -3 );

        TS_ASSERT( relu.satisfied() );

        relu.notifyVariableValue( b, 0 );

        TS_ASSERT( relu.satisfied() );

        relu.notifyVariableValue( f, 0 );
        relu.notifyVariableValue( b, 11 );

        TS_ASSERT( !relu.satisfied() );
    }

    void test_relu_fixes()
    {
        unsigned b = 1;
        unsigned f = 4;

        ReluConstraint relu( b, f );

        List<PiecewiseLinearConstraint::Fix> fixes;
        List<PiecewiseLinearConstraint::Fix>::iterator it;

        relu.notifyVariableValue( b, -1 );
        relu.notifyVariableValue( f, 1 );

        fixes = relu.getPossibleFixes();
        it = fixes.begin();
        TS_ASSERT_EQUALS( it->_variable, b );
        TS_ASSERT_EQUALS( it->_value, 1 );
        ++it;
        TS_ASSERT_EQUALS( it->_variable, f );
        TS_ASSERT_EQUALS( it->_value, 0 );

        relu.notifyVariableValue( b, 2 );
        relu.notifyVariableValue( f, 1 );

        fixes = relu.getPossibleFixes();
        it = fixes.begin();
        TS_ASSERT_EQUALS( it->_variable, b );
        TS_ASSERT_EQUALS( it->_value, 1 );
        ++it;
        TS_ASSERT_EQUALS( it->_variable, f );
        TS_ASSERT_EQUALS( it->_value, 2 );

        relu.notifyVariableValue( b, 11 );
        relu.notifyVariableValue( f, 0 );

        fixes = relu.getPossibleFixes();
        it = fixes.begin();
        TS_ASSERT_EQUALS( it->_variable, b );
        TS_ASSERT_EQUALS( it->_value, 0 );
        ++it;
        TS_ASSERT_EQUALS( it->_variable, f );
        TS_ASSERT_EQUALS( it->_value, 11 );
    }

    void test_relu_case_splits()
    {
        unsigned b = 1;
        unsigned f = 4;

        unsigned auxVar = 100;
        FreshVariables::setNextVariable( auxVar );

        ReluConstraint relu( b, f );

        List<PiecewiseLinearConstraint::Fix> fixes;
        List<PiecewiseLinearConstraint::Fix>::iterator it;

        List<PiecewiseLinearCaseSplit> splits = relu.getCaseSplits();

        Equation activeEquation, inactiveEquation;

        TS_ASSERT_EQUALS( splits.size(), 2U );

        // First split
        auto split = splits.begin();
        List<Tightening> bounds = split->getBoundTightenings();

        TS_ASSERT_EQUALS( bounds.size(), 3U );
        auto bound = bounds.begin();
        Tightening bound1 = *bound;
        ++bound;
        Tightening bound2 = *bound;
        ++bound;
        Tightening bound3 = *bound;

        TS_ASSERT_EQUALS( bound1._variable, b );
        TS_ASSERT_EQUALS( bound1._type, Tightening::LB );
        TS_ASSERT_EQUALS( bound1._value, 0.0 );

        TS_ASSERT_EQUALS( bound2._variable, auxVar );
        TS_ASSERT_EQUALS( bound2._type, Tightening::UB );
        TS_ASSERT_EQUALS( bound2._value, 0.0 );

        TS_ASSERT_EQUALS( bound3._variable, auxVar );
        TS_ASSERT_EQUALS( bound3._type, Tightening::LB );
        TS_ASSERT_EQUALS( bound3._value, 0.0 );

        List<Equation> equations = split->getEquations();
        TS_ASSERT_EQUALS( equations.size(), 1U );
        activeEquation = split->getEquations().front();
        TS_ASSERT_EQUALS( activeEquation._addends.size(), 3U );
        TS_ASSERT_EQUALS( activeEquation._scalar, 0.0 );

        auto addend = activeEquation._addends.begin();
        TS_ASSERT_EQUALS( addend->_coefficient, 1.0 );
        TS_ASSERT_EQUALS( addend->_variable, b );

        ++addend;
        TS_ASSERT_EQUALS( addend->_coefficient, -1.0 );
        TS_ASSERT_EQUALS( addend->_variable, f );

        ++addend;
        TS_ASSERT_EQUALS( addend->_coefficient, 1.0 );
        TS_ASSERT_EQUALS( addend->_variable, 100U );
        TS_ASSERT_EQUALS( activeEquation._auxVariable, 100U );

        // Second split
        ++split;
        bounds = split->getBoundTightenings();

        TS_ASSERT_EQUALS( bounds.size(), 3U );
        bound = bounds.begin();
        bound1 = *bound;
        ++bound;
        bound2 = *bound;
        ++bound;
        bound3 = *bound;

        TS_ASSERT_EQUALS( bound1._variable, b );
        TS_ASSERT_EQUALS( bound1._type, Tightening::UB );
        TS_ASSERT_EQUALS( bound1._value, 0.0 );

        TS_ASSERT_EQUALS( bound2._variable, auxVar );
        TS_ASSERT_EQUALS( bound2._type, Tightening::UB );
        TS_ASSERT_EQUALS( bound2._value, 0.0 );

        TS_ASSERT_EQUALS( bound3._variable, auxVar );
        TS_ASSERT_EQUALS( bound3._type, Tightening::LB );
        TS_ASSERT_EQUALS( bound3._value, 0.0 );

        equations = split->getEquations();
        TS_ASSERT_EQUALS( equations.size(), 1U );
        inactiveEquation = split->getEquations().front();
        TS_ASSERT_EQUALS( inactiveEquation._addends.size(), 2U );
        TS_ASSERT_EQUALS( inactiveEquation._scalar, 0.0 );

        addend = inactiveEquation._addends.begin();
        TS_ASSERT_EQUALS( addend->_coefficient, 1.0 );
        TS_ASSERT_EQUALS( addend->_variable, f );

        ++addend;
        TS_ASSERT_EQUALS( addend->_coefficient, 1.0 );
        TS_ASSERT_EQUALS( addend->_variable, 100U );
        TS_ASSERT_EQUALS( inactiveEquation._auxVariable, 100U );
    }

    void test_register_as_watcher()
    {
        unsigned b = 1;
        unsigned f = 4;

        MockTableau tableau;

        ReluConstraint relu( b, f );

        TS_ASSERT_THROWS_NOTHING( relu.registerAsWatcher( &tableau ) );

        TS_ASSERT_EQUALS( tableau.lastRegisteredVariableToWatcher.size(), 2U );
        TS_ASSERT( tableau.lastUnregisteredVariableToWatcher.empty() );
        TS_ASSERT_EQUALS( tableau.lastRegisteredVariableToWatcher[b].size(), 1U );
        TS_ASSERT( tableau.lastRegisteredVariableToWatcher[b].exists( &relu ) );
        TS_ASSERT_EQUALS( tableau.lastRegisteredVariableToWatcher[f].size(), 1U );
        TS_ASSERT( tableau.lastRegisteredVariableToWatcher[f].exists( &relu ) );

        tableau.lastRegisteredVariableToWatcher.clear();

        TS_ASSERT_THROWS_NOTHING( relu.unregisterAsWatcher( &tableau ) );

        TS_ASSERT( tableau.lastRegisteredVariableToWatcher.empty() );
        TS_ASSERT_EQUALS( tableau.lastUnregisteredVariableToWatcher.size(), 2U );
        TS_ASSERT_EQUALS( tableau.lastUnregisteredVariableToWatcher[b].size(), 1U );
        TS_ASSERT( tableau.lastUnregisteredVariableToWatcher[b].exists( &relu ) );
        TS_ASSERT_EQUALS( tableau.lastUnregisteredVariableToWatcher[f].size(), 1U );
        TS_ASSERT( tableau.lastUnregisteredVariableToWatcher[f].exists( &relu ) );
    }

    void test_fix_active()
    {
        unsigned b = 1;
        unsigned f = 4;

        MockTableau tableau;

        ReluConstraint relu( b, f );

        relu.registerAsWatcher( &tableau );

        List<PiecewiseLinearCaseSplit> splits = relu.getCaseSplits();
        TS_ASSERT_EQUALS( splits.size(), 2U );

        relu.notifyLowerBound( 1, 1.0 );
        TS_ASSERT_THROWS_EQUALS( splits = relu.getCaseSplits(),
                                 const ReluplexError &e,
                                 e.getCode(),
                                 ReluplexError::REQUESTED_CASE_SPLITS_FROM_FIXED_CONSTRAINT );

        relu.unregisterAsWatcher( &tableau );

        relu = ReluConstraint( b, f );

        relu.registerAsWatcher( &tableau );

        splits = relu.getCaseSplits();
        TS_ASSERT_EQUALS( splits.size(), 2U );

        relu.notifyLowerBound( 4, 1.0 );
        TS_ASSERT_THROWS_EQUALS( splits = relu.getCaseSplits(),
                                 const ReluplexError &e,
                                 e.getCode(),
                                 ReluplexError::REQUESTED_CASE_SPLITS_FROM_FIXED_CONSTRAINT );

        relu.unregisterAsWatcher( &tableau );
    }

    void test_fix_inactive()
    {
        unsigned b = 1;
        unsigned f = 4;

        MockTableau tableau;

        ReluConstraint relu( b, f );

        relu.registerAsWatcher( &tableau );

        List<PiecewiseLinearCaseSplit> splits = relu.getCaseSplits();
        TS_ASSERT_EQUALS( splits.size(), 2U );

        relu.notifyUpperBound( 4, -1.0 );
        TS_ASSERT_THROWS_EQUALS( splits = relu.getCaseSplits(),
                                  const ReluplexError &e,
                                  e.getCode(),
                                  ReluplexError::REQUESTED_CASE_SPLITS_FROM_FIXED_CONSTRAINT );

        relu.unregisterAsWatcher( &tableau );
    }

    void test_constraint_phase_gets_fixed()
    {
        unsigned b = 1;
        unsigned f = 4;

        MockTableau tableau;

        // Upper bounds
        {
            ReluConstraint relu( b, f );
            TS_ASSERT( !relu.phaseFixed() );
            relu.notifyUpperBound( b, -1.0 );
            TS_ASSERT( relu.phaseFixed() );
        }

        {
            ReluConstraint relu( b, f );
            TS_ASSERT( !relu.phaseFixed() );
            relu.notifyUpperBound( b, 0.0 );
            TS_ASSERT( relu.phaseFixed() );
        }

        {
            ReluConstraint relu( b, f );
            TS_ASSERT( !relu.phaseFixed() );
            relu.notifyUpperBound( f, 0.0 );
            TS_ASSERT( relu.phaseFixed() );
        }

        {
            ReluConstraint relu( b, f );
            TS_ASSERT( !relu.phaseFixed() );
            relu.notifyUpperBound( b, 3.0 );
            TS_ASSERT( !relu.phaseFixed() );
        }

        {
            ReluConstraint relu( b, f );
            TS_ASSERT( !relu.phaseFixed() );
            relu.notifyUpperBound( b, 5.0 );
            TS_ASSERT( !relu.phaseFixed() );
        }

        // Lower bounds
        {
            ReluConstraint relu( b, f );
            TS_ASSERT( !relu.phaseFixed() );
            relu.notifyLowerBound( b, 3.0 );
            TS_ASSERT( relu.phaseFixed() );
        }

        {
            ReluConstraint relu( b, f );
            TS_ASSERT( !relu.phaseFixed() );
            relu.notifyLowerBound( b, 0.0 );
            TS_ASSERT( relu.phaseFixed() );
        }

        {
            ReluConstraint relu( b, f );
            TS_ASSERT( !relu.phaseFixed() );
            relu.notifyLowerBound( f, 6.0 );
            TS_ASSERT( relu.phaseFixed() );
        }

        {
            ReluConstraint relu( b, f );
            TS_ASSERT( !relu.phaseFixed() );
            relu.notifyLowerBound( f, 0.0 );
            TS_ASSERT( !relu.phaseFixed() );
        }

        {
            ReluConstraint relu( b, f );
            TS_ASSERT( !relu.phaseFixed() );
            relu.notifyLowerBound( b, -2.0 );
            TS_ASSERT( !relu.phaseFixed() );
        }
    }

    void test_valid_split_relu_phase_fixed_to_active()
    {
        unsigned b = 1;
        unsigned f = 4;

        unsigned auxVar = 100;
        FreshVariables::setNextVariable( auxVar );

        ReluConstraint relu( b, f );

        List<PiecewiseLinearConstraint::Fix> fixes;
        List<PiecewiseLinearConstraint::Fix>::iterator it;

        TS_ASSERT( !relu.phaseFixed() );
        TS_ASSERT_THROWS_NOTHING( relu.notifyLowerBound( b, 5 ) );
        TS_ASSERT( relu.phaseFixed() );

        PiecewiseLinearCaseSplit split;
        TS_ASSERT_THROWS_NOTHING( split = relu.getValidCaseSplit() );

        Equation activeEquation;

        List<Tightening> bounds = split.getBoundTightenings();

        TS_ASSERT_EQUALS( bounds.size(), 3U );
        auto bound = bounds.begin();
        Tightening bound1 = *bound;
        ++bound;
        Tightening bound2 = *bound;
        ++bound;
        Tightening bound3 = *bound;

        TS_ASSERT_EQUALS( bound1._variable, b );
        TS_ASSERT_EQUALS( bound1._type, Tightening::LB );
        TS_ASSERT_EQUALS( bound1._value, 0.0 );

        TS_ASSERT_EQUALS( bound2._variable, auxVar );
        TS_ASSERT_EQUALS( bound2._type, Tightening::UB );
        TS_ASSERT_EQUALS( bound2._value, 0.0 );

        TS_ASSERT_EQUALS( bound3._variable, auxVar );
        TS_ASSERT_EQUALS( bound3._type, Tightening::LB );
        TS_ASSERT_EQUALS( bound3._value, 0.0 );

        List<Equation> equations = split.getEquations();
        TS_ASSERT_EQUALS( equations.size(), 1U );
        activeEquation = split.getEquations().front();
        TS_ASSERT_EQUALS( activeEquation._addends.size(), 3U );
        TS_ASSERT_EQUALS( activeEquation._scalar, 0.0 );

        auto addend = activeEquation._addends.begin();
        TS_ASSERT_EQUALS( addend->_coefficient, 1.0 );
        TS_ASSERT_EQUALS( addend->_variable, b );

        ++addend;
        TS_ASSERT_EQUALS( addend->_coefficient, -1.0 );
        TS_ASSERT_EQUALS( addend->_variable, f );

        ++addend;
        TS_ASSERT_EQUALS( addend->_coefficient, 1.0 );
        TS_ASSERT_EQUALS( addend->_variable, 100U );
        TS_ASSERT_EQUALS( activeEquation._auxVariable, 100U );
    }

    void test_valid_split_relu_phase_fixed_to_inactive()
    {
        unsigned b = 1;
        unsigned f = 4;

        unsigned auxVar = 100;
        FreshVariables::setNextVariable( auxVar );

        ReluConstraint relu( b, f );

        List<PiecewiseLinearConstraint::Fix> fixes;
        List<PiecewiseLinearConstraint::Fix>::iterator it;

        TS_ASSERT( !relu.phaseFixed() );
        TS_ASSERT_THROWS_NOTHING( relu.notifyUpperBound( b, -2 ) );
        TS_ASSERT( relu.phaseFixed() );

        PiecewiseLinearCaseSplit split;
        TS_ASSERT_THROWS_NOTHING( split = relu.getValidCaseSplit() );

        Equation activeEquation;

        List<Tightening> bounds = split.getBoundTightenings();

        TS_ASSERT_EQUALS( bounds.size(), 3U );
        auto bound = bounds.begin();
        Tightening bound1 = *bound;
        ++bound;
        Tightening bound2 = *bound;
        ++bound;
        Tightening bound3 = *bound;

        TS_ASSERT_EQUALS( bound1._variable, b );
        TS_ASSERT_EQUALS( bound1._type, Tightening::UB );
        TS_ASSERT_EQUALS( bound1._value, 0.0 );

        TS_ASSERT_EQUALS( bound2._variable, auxVar );
        TS_ASSERT_EQUALS( bound2._type, Tightening::UB );
        TS_ASSERT_EQUALS( bound2._value, 0.0 );

        TS_ASSERT_EQUALS( bound3._variable, auxVar );
        TS_ASSERT_EQUALS( bound3._type, Tightening::LB );
        TS_ASSERT_EQUALS( bound3._value, 0.0 );

        List<Equation> equations = split.getEquations();
        TS_ASSERT_EQUALS( equations.size(), 1U );
        Equation inactiveEquation = split.getEquations().front();
        TS_ASSERT_EQUALS( inactiveEquation._addends.size(), 2U );
        TS_ASSERT_EQUALS( inactiveEquation._scalar, 0.0 );

        auto addend = inactiveEquation._addends.begin();
        TS_ASSERT_EQUALS( addend->_coefficient, 1.0 );
        TS_ASSERT_EQUALS( addend->_variable, f );

        ++addend;
        TS_ASSERT_EQUALS( addend->_coefficient, 1.0 );
        TS_ASSERT_EQUALS( addend->_variable, 100U );
        TS_ASSERT_EQUALS( inactiveEquation._auxVariable, 100U );
    }

    void test_relu_store_and_restore()
    {
        unsigned b = 1;
        unsigned f = 4;

        ReluConstraint relu( b, f );

        relu.notifyVariableValue( b, 1 );
        relu.notifyVariableValue( f, 2 );

        PiecewiseLinearConstraintState *state = relu.allocateState();
        relu.storeState( *state );
        ReluConstraintState *reluState = dynamic_cast<ReluConstraintState *>( state );

        TS_ASSERT( reluState->_constraintActive );
        TS_ASSERT_EQUALS( reluState->_assignment[b], 1 );
        TS_ASSERT_EQUALS( reluState->_assignment[f], 2 );
        TS_ASSERT_EQUALS( reluState->_phaseStatus, ReluConstraint::PhaseStatus::PHASE_NOT_FIXED );

        relu.setActiveConstraint( false );
        relu.notifyVariableValue( b, 3 );
        relu.notifyVariableValue( f, 4 );
        relu.notifyLowerBound( f, 1 );

        PiecewiseLinearConstraintState *state2 = relu.allocateState();
        relu.storeState( *state2 );
        ReluConstraintState *reluState2 = dynamic_cast<ReluConstraintState *>( state2 );

        TS_ASSERT( !reluState2->_constraintActive );
        TS_ASSERT_EQUALS( reluState2->_assignment[b], 3 );
        TS_ASSERT_EQUALS( reluState2->_assignment[f], 4 );
        TS_ASSERT_EQUALS( reluState2->_phaseStatus, ReluConstraint::PhaseStatus::PHASE_ACTIVE );

        relu.restoreState( *state );
        TS_ASSERT( reluState->_constraintActive );
        TS_ASSERT_EQUALS( reluState->_assignment[b], 1 );
        TS_ASSERT_EQUALS( reluState->_assignment[f], 2 );
        TS_ASSERT_EQUALS( reluState->_phaseStatus, ReluConstraint::PhaseStatus::PHASE_NOT_FIXED );

        relu.restoreState( *state2 );
        TS_ASSERT( !reluState2->_constraintActive );
        TS_ASSERT_EQUALS( reluState2->_assignment[b], 3 );
        TS_ASSERT_EQUALS( reluState2->_assignment[f], 4 );
        TS_ASSERT_EQUALS( reluState2->_phaseStatus, ReluConstraint::PhaseStatus::PHASE_ACTIVE );

    }
};

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
