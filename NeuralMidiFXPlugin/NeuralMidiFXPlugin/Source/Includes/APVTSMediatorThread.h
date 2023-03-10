//
// Created by ANON on 2022-09-02.
//

#pragma once

#include "shared_plugin_helpers/shared_plugin_helpers.h"
#include "../DeploymentSettings/ThreadsAndQueuesAndInputEvents.h"
#include "GuiParameters.h"
#include "../Includes/LockFreeQueue.h"

#pragma once


// ============================================================================================================
// ==========         This Thread is in charge of checking which parameters in APVTS have been changed.
// ==========           If changed, the updated value will be pushed to a corresponding queue to be read
// ==========           by the receiving/destination thread.
// ==========
// ==========         To read from APVTS, we always get a std::atomic pointer, which potentially
// ==========           can block a thread if some read/write race is happening. As a result, it is not
// ==========           safe to directly read from APVTS inside the processBlock() thread or any other
// ==========           time sensitive threads
// ==========
// ==========         In this plugin, the GrooveThread and the ModelThread are not time-sensitive. So,
// ==========           they can read from APVTS directly. Regardless, in future iterations, perhaps
// ==========           these requirements change. To be future-proof, this thread has been implemented
// ==========           to take care of mediating the communication of parameters in the APVTS to the
// ==========           DeploymentThreads as well as the processBlock()
// ============================================================================================================
class APVTSMediatorThread: public juce::Thread
{
public:
    juce::StringArray paths;

    // I want to declare these as private, but if I do it lower down it doesn't work..
    size_t numSliders{};
    size_t numRotaries{};


    // ============================================================================================================
    // ===          Preparing Thread for Running
    // ============================================================================================================
    // ------------------------------------------------------------------------------------------------------------
    // ---         Step 1 . Construct
    // ------------------------------------------------------------------------------------------------------------
    APVTSMediatorThread() : juce::Thread("APVTSMediatorThread") {}

    // ------------------------------------------------------------------------------------------------------------
    // ---         Step 2 . give access to resources needed to communicate with other threads
    // ------------------------------------------------------------------------------------------------------------
    void startThreadUsingProvidedResources(
            juce::AudioProcessorValueTreeState *APVTSPntr_,
            LockFreeQueue<GuiParams, queue_settings::APVM_que_size> *APVM2ITP_GuiParams_QuePntr_,
            LockFreeQueue<GuiParams, queue_settings::APVM_que_size> *APVM2MDL_GuiParams_QuePntr_,
            LockFreeQueue<GuiParams, queue_settings::APVM_que_size> *APVM2PPP_GuiParams_QuePntr_) {

        // Resources Provided from NMP
        APVTSPntr = APVTSPntr_;
        APVM2ITP_GuiParams_QuePntr = APVM2ITP_GuiParams_QuePntr_;
        APVM2MDL_GuiParams_QuePntr = APVM2MDL_GuiParams_QuePntr_;
        APVM2PPP_GuiParams_QuePntr = APVM2PPP_GuiParams_QuePntr_;

        guiParamsPntr = make_unique<GuiParams>(APVTSPntr_);
        // Get UIObjects in settings.h
        auto tabList = UIObjects::Tabs::tabList;
        size_t numTabs = tabList.size();

        startThread();
    }

    // ------------------------------------------------------------------------------------------------------------
    // ---         Step 3 . start run() thread by calling startThread().
    // ---                  !!DO NOT!! Call run() directly. startThread() internally makes a call to run().
    // ---                  (Implement what the thread does inside the run() method
    // ------------------------------------------------------------------------------------------------------------
    void run() override {
        // notify if the thread is still running
        bool bExit = threadShouldExit();

        while (!bExit) {
            if (APVTSPntr != nullptr) {
                if (guiParamsPntr->update(APVTSPntr)) {
                    // guiParamsPntr->print();
                    APVM2ITP_GuiParams_QuePntr->push(*guiParamsPntr);
                    APVM2MDL_GuiParams_QuePntr->push(*guiParamsPntr);
                    APVM2PPP_GuiParams_QuePntr->push(*guiParamsPntr);
                }

                bExit = threadShouldExit();

                // avoid burning CPU, if reading is returning immediately
                sleep(thread_configurations::APVTSMediatorThread::waitTimeBtnIters);
            }
        }
    }
    // ============================================================================================================


    // ============================================================================================================
    // ===          Preparing Thread for Stopping
    // ============================================================================================================
    bool readyToStop{false}; // Used to check if thread is ready to be stopped or externally stopped from a parent thread

    // run this in destructor destructing object
    void prepareToStop() {
        //Need to wait enough to ensure the run() method is over before killing thread
        this->stopThread(100 * thread_configurations::APVTSMediatorThread::waitTimeBtnIters);
        readyToStop = true;
    }

    ~APVTSMediatorThread() override {
        if (not readyToStop) {
            prepareToStop();
        }
    }

private:

    // ============================================================================================================
    // ===          Output Queues for Receiving/Sending Data
    // ============================================================================================================
    LockFreeQueue<GuiParams, queue_settings::APVM_que_size> *APVM2ITP_GuiParams_QuePntr{nullptr};
    LockFreeQueue<GuiParams, queue_settings::APVM_que_size> *APVM2MDL_GuiParams_QuePntr{nullptr};
    LockFreeQueue<GuiParams, queue_settings::APVM_que_size> *APVM2PPP_GuiParams_QuePntr{nullptr};

    unique_ptr<GuiParams> guiParamsPntr;

//    // ============================================================================================================
//    // ===          pointer to NeuralMidiFXPluginProcessor
//    // ============================================================================================================
//    InputTensorPreparatorThread *inputThread{nullptr};
//    ModelThread *modelThread{nullptr};
//    PlaybackPreparatorThread *playbackPreparatorThread{nullptr};

    // ============================================================================================================
    // ===          Pointer to APVTS hosted in the Main Processor
    // ============================================================================================================
    juce::AudioProcessorValueTreeState *APVTSPntr{nullptr};
};
