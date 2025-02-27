/****************************************************************************/
// Eclipse SUMO, Simulation of Urban MObility; see https://eclipse.org/sumo
// Copyright (C) 2001-2019 German Aerospace Center (DLR) and others.
// This program and the accompanying materials
// are made available under the terms of the Eclipse Public License v2.0
// which accompanies this distribution, and is available at
// http://www.eclipse.org/legal/epl-v20.html
// SPDX-License-Identifier: EPL-2.0
/****************************************************************************/
/// @file    MSCFModel_CACC.cpp
/// @author  Kallirroi Porfyri
/// @date    Nov 2018
/// @version $Id$
///
// CACC car-following model based on [1], [2].
// [1] Milanes, V., and S. E. Shladover. Handling Cut-In Vehicles in Strings
//    of Cooperative Adaptive Cruise Control Vehicles. Journal of Intelligent
//     Transportation Systems, Vol. 20, No. 2, 2015, pp. 178-191.
// [2] Xiao, L., M. Wang and B. van Arem. Realistic Car-Following Models for
//    Microscopic Simulation of Adaptive and Cooperative Adaptive Cruise
//     Control Vehicles. Transportation Research Record: Journal of the
//     Transportation Research Board, No. 2623, 2017. (DOI: 10.3141/2623-01).
/****************************************************************************/

// ===========================================================================
// included modules
// ===========================================================================
#include <config.h>

#include <stdio.h>
#include <iostream>

#include "MSCFModel_CACC.h"
#include <microsim/MSVehicle.h>
#include <microsim/MSLane.h>
#include <utils/common/RandHelper.h>
#include <utils/common/SUMOTime.h>
#include <microsim/lcmodels/MSAbstractLaneChangeModel.h>
#include <math.h>
#include <microsim/MSNet.h>

// ===========================================================================
// debug flags
// ===========================================================================
#define DEBUG_CACC 0
#define DEBUG_COND (veh->isSelected())


// ===========================================================================
// defaults
// ===========================================================================
#define DEFAULT_SC_GAIN_CACC -0.4
#define DEFAULT_GCC_GAIN_GAP_CACC 0.005
#define DEFAULT_GCC_GAIN_GAP_DOT_CACC 0.05
#define DEFAULT_GC_GAIN_GAP_CACC 0.45
#define DEFAULT_GC_GAIN_GAP_DOT_CACC 0.0125
#define DEFAULT_CA_GAIN_GAP_CACC 0.45
#define DEFAULT_CA_GAIN_GAP_DOT_CACC 0.05

// override followSpeed when deemed unsafe by the given margin (the value was selected to reduce the number of necessary interventions)
#define DEFAULT_EMERGENCY_OVERRIDE_THRESHOLD 2.0


// ===========================================================================
// method definitions
// ===========================================================================
MSCFModel_CACC::MSCFModel_CACC(const MSVehicleType* vtype) :
    MSCFModel(vtype), acc_CFM(MSCFModel_ACC(vtype)),
    mySpeedControlGain(vtype->getParameter().getCFParam(SUMO_ATTR_SC_GAIN_CACC, DEFAULT_SC_GAIN_CACC)),
    myGapClosingControlGainGap(vtype->getParameter().getCFParam(SUMO_ATTR_GCC_GAIN_GAP_CACC, DEFAULT_GCC_GAIN_GAP_CACC)),
    myGapClosingControlGainGapDot(vtype->getParameter().getCFParam(SUMO_ATTR_GCC_GAIN_GAP_DOT_CACC, DEFAULT_GCC_GAIN_GAP_DOT_CACC)),
    myGapControlGainGap(vtype->getParameter().getCFParam(SUMO_ATTR_GC_GAIN_GAP_CACC, DEFAULT_GC_GAIN_GAP_CACC)),
    myGapControlGainGapDot(vtype->getParameter().getCFParam(SUMO_ATTR_GC_GAIN_GAP_DOT_CACC, DEFAULT_GC_GAIN_GAP_DOT_CACC)),
    myCollisionAvoidanceGainGap(vtype->getParameter().getCFParam(SUMO_ATTR_CA_GAIN_GAP_CACC, DEFAULT_CA_GAIN_GAP_CACC)),
    myCollisionAvoidanceGainGapDot(vtype->getParameter().getCFParam(SUMO_ATTR_CA_GAIN_GAP_DOT_CACC, DEFAULT_CA_GAIN_GAP_DOT_CACC)) {
    myCollisionMinGapFactor = vtype->getParameter().getCFParam(SUMO_ATTR_COLLISION_MINGAP_FACTOR, 0.1);
}

MSCFModel_CACC::~MSCFModel_CACC() {}


double
MSCFModel_CACC::followSpeed(const MSVehicle* const veh, double speed, double gap2pred, double predSpeed, double predMaxDecel, const MSVehicle* const /*pred*/) const {

    const double desSpeed = MIN2(veh->getLane()->getSpeedLimit(), veh->getMaxSpeed());
    const double vCACC = _v(veh, gap2pred, speed, predSpeed, desSpeed, true);
    const double vSafe = maximumSafeFollowSpeed(gap2pred, speed, predSpeed, predMaxDecel);
    if (vSafe + DEFAULT_EMERGENCY_OVERRIDE_THRESHOLD < vCACC) {
        if DEBUG_COND {
        CACCVehicleVariables* vars = (CACCVehicleVariables*)veh->getCarFollowVariables();
            std::cout << "\n";
            std::cout << "Apply Safe speed" << "\n";
            std::cout << SIMTIME << " veh=" << veh->getID() << " v=" << speed << " vL=" << predSpeed << " gap=" << gap2pred << " vCACC=" << vCACC << " vSafe=" << vSafe << " cm=" << vars->CACC_ControlMode << "\n";
        }
        return vSafe + DEFAULT_EMERGENCY_OVERRIDE_THRESHOLD;
    }
    return vCACC;
}

double
MSCFModel_CACC::stopSpeed(const MSVehicle* const veh, const double speed, double gap) const {

    // NOTE: This allows return of smaller values than minNextSpeed().
    // Only relevant for the ballistic update: We give the argument headway=TS, to assure that
    // the stopping position is approached with a uniform deceleration also for tau!=TS.
    return MIN2(maximumSafeStopSpeed(gap, speed, false, veh->getActionStepLengthSecs()), maxNextSpeed(speed, veh));
}

double
MSCFModel_CACC::getSecureGap(const double speed, const double leaderSpeed, const double /* leaderMaxDecel */) const {
    // Accel in gap mode should vanish:
    //      0 = myGapControlGainSpeed * (leaderSpeed - speed) + myGapControlGainSpace * (g - myHeadwayTime * speed);
    // <=>  myGapControlGainSpace * g = - myGapControlGainSpeed * (leaderSpeed - speed) + myGapControlGainSpace * myHeadwayTime * speed;
    // <=>  g = - myGapControlGainSpeed * (leaderSpeed - speed) / myGapControlGainSpace + myHeadwayTime * speed;
    return acc_CFM.myGapControlGainSpeed * (speed - leaderSpeed) / acc_CFM.myGapControlGainSpace + myHeadwayTime * speed;
}

double
MSCFModel_CACC::insertionFollowSpeed(const MSVehicle* const v, double speed, double gap2pred, double predSpeed, double predMaxDecel) const {
//#ifdef DEBUG_CACC
//        std::cout << "MSCFModel_ACC::insertionFollowSpeed(), speed="<<speed<< std::endl;
//#endif
    // iterate to find a stationary value for
    //    speed = followSpeed(v, speed, gap2pred, predSpeed, predMaxDecel, nullptr)
    const int max_iter = 50;
    int n_iter = 0;
    const double tol = 0.1;
    const double damping = 0.1;

    double res = speed;
    while (n_iter < max_iter) {
        // proposed acceleration
        const double a = SPEED2ACCEL(followSpeed(v, res, gap2pred, predSpeed, predMaxDecel, nullptr) - res);
        res = res + damping * a;
//#ifdef DEBUG_CACC
//        std::cout << "   n_iter=" << n_iter << ", a=" << a << ", res=" << res << std::endl;
//#endif
        if (fabs(a) < tol) {
            break;
        } else {
            n_iter++;
        }
    }
    return res;
}




/// @todo update interactionGap logic
double
MSCFModel_CACC::interactionGap(const MSVehicle* const /* veh */, double /* vL */) const {
    /*maximum radar range is CACC is enabled*/
    return 250;
}

double MSCFModel_CACC::speedSpeedContol(const double speed, double vErr) const {
    // Speed control law
    double sclAccel = mySpeedControlGain * vErr;
    double newSpeed = speed + ACCEL2SPEED(sclAccel);
    return newSpeed;
}

double MSCFModel_CACC::speedGapControl(const MSVehicle* const veh, const double gap2pred,
                                       const double speed, const double predSpeed, const double desSpeed, double vErr) const {
    // Gap control law
    double newSpeed = 0.0;

    std::pair<const MSVehicle* const, double> leaderInfo = veh->getLeader(100);
    if (leaderInfo.first) {
        if (leaderInfo.first->getCarFollowModel().getModelID() != SUMO_TAG_CF_CACC) {
            //ACC control mode
            newSpeed = acc_CFM._v(veh, gap2pred, speed, predSpeed, desSpeed, true);

        } else {
            //CACC control mode
            double desSpacing = myHeadwayTime * speed;
            double gap = gap2pred - veh->getVehicleType().getMinGap();
            double spacingErr = gap - desSpacing;
            double accel = veh->getAcceleration();
            double spacingErr1 = predSpeed - speed + myHeadwayTime * accel;

            if ((spacingErr > 0 && spacingErr < 0.2) && (vErr < 0.1)) {
                // gap mode
                //newSpeed = speed + 0.45 * spacingErr + 0.0125 *spacingErr1;
#if DEBUG_CACC == 1
                if DEBUG_COND {
                std::cout << "        applying gap control" << std::endl;
            }
#endif
            newSpeed = speed + myGapControlGainGap * spacingErr + myGapControlGainGapDot * spacingErr1;
        } else if (spacingErr < 0)  {
                // collision avoidance mode
                //newSpeed = speed + 0.45 * spacingErr + 0.05 *spacingErr1;
#if DEBUG_CACC == 1
                if DEBUG_COND {
                std::cout << "        applying collision avoidance" << std::endl;
            }
#endif
            newSpeed = speed + myCollisionAvoidanceGainGap * spacingErr + myCollisionAvoidanceGainGapDot * spacingErr1;
        } else {
            // gap closing mode
#if DEBUG_CACC == 1
            if DEBUG_COND {
                std::cout << "        applying gap closing" << std::endl;
            }
#endif
            newSpeed = speed + myGapClosingControlGainGap * spacingErr + myGapClosingControlGainGapDot * spacingErr1;
        }
    }

} else { /* no leader */
    newSpeed = speedSpeedContol(speed, vErr);
    }

    return newSpeed;

}

double
MSCFModel_CACC::_v(const MSVehicle* const veh, const double gap2pred, const double speed,
                   const double predSpeed, const double desSpeed, const bool /* respectMinGap */) const {

    double newSpeed = 0.0;

#if DEBUG_CACC == 1
    if DEBUG_COND {
    std::cout << SIMTIME << " MSCFModel_CACC::_v() for veh '" << veh->getID() << "'\n"
                  << "        gap=" << gap2pred << " speed="  << speed << " predSpeed=" << predSpeed
                  << " desSpeed=" << desSpeed << std::endl;
    }
#endif

    /* Velocity error */
    double vErr = speed - desSpeed;
    int setControlMode = 0;
    CACCVehicleVariables* vars = (CACCVehicleVariables*)veh->getCarFollowVariables();
    if (vars->lastUpdateTime != MSNet::getInstance()->getCurrentTimeStep()) {
        vars->lastUpdateTime = MSNet::getInstance()->getCurrentTimeStep();
        setControlMode = 1;
    }

    double time_gap = gap2pred / speed;
    if (time_gap > 2) {
#if DEBUG_CACC == 1
        if DEBUG_COND {
        std::cout << "        applying speedControl" << std::endl;
    }
#endif
    // Find acceleration - Speed control law
    newSpeed = speedSpeedContol(speed, vErr);
        // Set cl to vehicle parameters
        if (setControlMode) {
            vars->CACC_ControlMode = 0;
        }
    } else if (time_gap < 1.5) {
        // Find acceleration - Gap control law
        newSpeed = speedGapControl(veh, gap2pred, speed, predSpeed, desSpeed, vErr);
        // Set cl to vehicle parameters
        if (setControlMode) {
            vars->CACC_ControlMode = 1;
        }
    } else {
        // Follow previous applied law
        int cm = vars->CACC_ControlMode;
        if (!cm) {

#if DEBUG_CACC == 1
            if DEBUG_COND {
            std::cout << "        applying speedControl" << std::endl;
        }
#endif
        newSpeed = speedSpeedContol(speed, vErr);
        } else {
            newSpeed = speedGapControl(veh, gap2pred, speed, predSpeed, desSpeed, vErr);
        }
    }

#if DEBUG_CACC == 1
    if DEBUG_COND {
    std::cout << "        result: accel=" << SPEED2ACCEL(newSpeed - speed) << " newSpeed="  << newSpeed << std::endl;
    }
#endif

    return MAX2(0., newSpeed);
}



MSCFModel*
MSCFModel_CACC::duplicate(const MSVehicleType* vtype) const {
    return new MSCFModel_CACC(vtype);
}

