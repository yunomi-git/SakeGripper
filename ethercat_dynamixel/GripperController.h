#ifndef __GRIPPER__
#define __GRIPPER__

// ECat Gripper interfaces between ecat and the LL gripper

#include <DynamixelShield.h>
#include "Timer.h"
#include "EcatCommunication.h"
#include "LowLevelGripper.h"
#include "MathUtil.h"

// Manages single gripper. Ties together ecat and dynamixel

class GripperController {
private:
    LowLevelGripper llGripper;
    EcatCommandSignal lastCommandSignal = WAITING;
    EcatCommandInfo ecatCommandInfo = EcatCommandInfo();
    EcatReplyInfo ecatReplyInfo = EcatReplyInfo();

public:
    GripperController() {
    }

    GripperController(int dxlId, DynamixelShield *ndxl) {
        llGripper = LowLevelGripper(dxlId, ndxl);
    }

    void sendEcatCommand(EcatCommandInfo necatCommandInfo) {
        ecatCommandInfo = necatCommandInfo;
    }

    EcatReplyInfo getReplyInfo() {
        ecatReplyInfo.busy = llGripper.isBusy();
        ecatReplyInfo.position = convertRatioToEcat(llGripper.getPositionRatio());
        ecatReplyInfo.torque = convertRatioToEcat(llGripper.getTorqueRatioMagnitude());
        ecatReplyInfo.temperature = llGripper.getTemperature();
        ecatReplyInfo.error = llGripper.getError();
        return ecatReplyInfo;
    }

    void doControl() {
        llGripper.operate();
        bool newCommandWasSent = (lastCommandSignal != ecatCommandInfo.command) 
            && (ecatCommandInfo.command != EcatCommandSignal::WAITING);

//        if (!llGripper.isBusy() && newCommandWasSent) {
        if (newCommandWasSent || ecatCommandInfo.command == EcatCommandSignal::GOTO) {
            executeCommand(ecatCommandInfo);
        }
        lastCommandSignal = ecatCommandInfo.command;
    }

    float convertEcatToRatio(int ecatValue) {
        float percent = (1.0 * ecatValue) / ECAT_RESOLUTION;
        return fconstrain(percent, 0.0, 1.0);
    }

    int convertRatioToEcat(float ratio) {
        int value = (int) (ratio * ECAT_RESOLUTION);
        return fconstrain(value, 0, ECAT_RESOLUTION);
    }

    void setZero(int zero) {
        llGripper.setZero(zero);
    }

private:
    void executeCommand(EcatCommandInfo ecatCommandInfo) {
        float positionRatio;
        float torqueRatio;
        switch (ecatCommandInfo.command) {
            case CALIBRATE:
                DEBUG_SERIAL.println("execute calibrate");
                llGripper.calibrate();
                break;
            case GOTO:
                positionRatio = convertEcatToRatio(ecatCommandInfo.position);
                torqueRatio = convertEcatToRatio(ecatCommandInfo.torque);
                llGripper.gotoPositionWithTorque(positionRatio, torqueRatio);
                DEBUG_SERIAL.println("execute goto");
                break;
            case RELEASE:
                llGripper.removeTorque();
                DEBUG_SERIAL.println("execute release");
                break;
            case OPEN:
                llGripper.open();
                DEBUG_SERIAL.println("execute open");
                break;
            case SET_TORQUE:
                torqueRatio = convertEcatToRatio(ecatCommandInfo.torque);
                llGripper.setTorque(torqueRatio);
                DEBUG_SERIAL.println("execute open");
                break;
            default:
                break;
        }
    }
};

#endif
