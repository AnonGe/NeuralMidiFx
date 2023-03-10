//
// Created by ANON on 2022-12-13.
//

#include "PlaybackPreparatorThread.h"


// ===================================================================================
// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
//                       YOUR IMPLEMENTATION SHOULD BE DONE HERE
// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// ===================================================================================
std::pair<bool, bool> PlaybackPreparatorThread::deploy(bool new_model_output_received, bool did_any_gui_params_change) {

    // =================================================================================
    // ===         1. ACCESSING GUI PARAMETERS
    // =================================================================================

    /* **NOTE**
        If you need to access information from the GUI, you can do so by using the
        following methods:

          Rotary/Sliders: gui_params.getValueFor([slider/button name])
          Toggle Buttons: gui_params.isToggleButtonOn([button name])
          Trigger Buttons: gui_params.wasButtonClicked([button name])
    **NOTE**
       If you only need this data when the GUI parameters CHANGE, you can use the
          provided gui_params_changed_since_last_call flag */

    // ---------------------------------------------------------------------------------
    // --- ExampleStarts ------ ExampleStarts ------ ExampleStarts ---- ExampleStarts --
    // in this example, whenever the slider is moved, I resend a sequence of generations
    // to the processor thread (NMP)
    bool testFlag{false};
    auto newPlaybackDelaySlider = gui_params.getValueFor("Generation Playback Delay");
    if (PlaybackDelaySlider != newPlaybackDelaySlider) {
        PrintMessage("PlaybackPreparatorThread: PlaybackDelaySlider changed from" +
        std::to_string(PlaybackDelaySlider) + " to " + std::to_string(newPlaybackDelaySlider));
        PlaybackDelaySlider = newPlaybackDelaySlider;
        testFlag = true;
    }
    // --- ExampleEnds -------- ExampleEnds -------- ExampleEnds ------ ExampleEnds ----
    // ---------------------------------------------------------------------------------

    // =================================================================================
    // ===         Step 2 . Extract NoteOn/Off/CC events from the generation data
    // =================================================================================

    // ---------------------------------------------------------------------------------
    // --- ExampleStarts ------ ExampleStarts ------ ExampleStarts ---- ExampleStarts --
    auto onsets = model_output.tensor1; // I'm not using this tensor below, just an e.g.
    // ...
    // --- ExampleEnds -------- ExampleEnds -------- ExampleEnds ------ ExampleEnds ----
    // ---------------------------------------------------------------------------------

    // =================================================================================
    // ===         Step 3 . At least once, before sending generations,
    //                  Specify the PlaybackPolicy, Time_unit, OverwritePolicy
    // =================================================================================

    /*
     * Here you Notify the main NMP thread that a new stream of generations is available.
     * Moreover, you also specify what time_unit is used for the generations (eg. sec, ppq, samples)
     * and also specify the OverwritePolicy (whether to delete all previously generated events
     * or to keep them while also adding the new ones in parallel, or only clear the events after
     * the time at which the current event is received)

     *
     * PlaybackPolicy:
     *      1. relative to Now (register the current time, and play messages relative to that)
     *      2. relative to 0 (stream should be played relative to absolute 0 time)
     *      3. relative to playback start (stream should be played relative to the time when playback started)
     *
     * Time_unit:
     *   1. seconds
     *   2. ppq
     *   3. audio samples
     *
     * Overwrite Policy:
     *  1. delete all events in previous stream and use new stream
     *  2. delete all events after now
     *  3. Keep all previous events (that is generations can be played on top of each other)
     */

    // -----------------------------------------------------------------------------------------
    // ------ ExampleStarts ------ ExampleStarts ------ ExampleStarts ------ ExampleStarts -----
    bool newPlaybackPolicyShouldBeSent{false};
    bool newPlaybackSequenceGeneratedAndShouldBeSent{false};

    if (testFlag) {
        // 1. ---- Update Playback Policy -----
        // Can be sent just once, or every time the policy changes
        // playbackPolicy.SetPaybackPolicy_RelativeToNow();  // or
        playbackPolicy.SetPlaybackPolicy_RelativeToAbsoluteZero(); // or
        // playbackPolicy.SetPplaybackPolicy_RelativeToPlaybackStart(); // or

        // playbackPolicy.SetTimeUnitIsSeconds(); // or
        playbackPolicy.SetTimeUnitIsPPQ(); // or
        // playbackPolicy.SetTimeUnitIsAudioSamples(); // or

        playbackPolicy.SetOverwritePolicy_DeleteAllEventsInPreviousStreamAndUseNewStream(); // or
        // playbackPolicy.SetOverwritePolicy_DeleteAllEventsAfterNow(); // or
        // playbackPolicy.SetOverwritePolicy_KeepAllPreviousEvents(); // or

        newPlaybackPolicyShouldBeSent = true;

        // 2. ---- Update Playback Sequence -----
        // clear the previous sequence or append depending on your requirements
        playbackSequence.clear();

        // I'm generating a single note to be played at timestamp 4 and delayed by the slider value
        int channel{1};
        int note{24};
        float velocity{0.3f};
        double timestamp{4};
        double duration{3};

        // add noteOn Offs (time stamp shifted by the slider value)
        playbackSequence.addNoteOn(channel, note, velocity,
                                   timestamp + newPlaybackDelaySlider);
        playbackSequence.addNoteOff(channel, note, velocity,
                                    timestamp + newPlaybackDelaySlider + duration);
        // or add a note with duration (an octave higher)
        playbackSequence.addNoteWithDuration(channel, note + 12, velocity,
                                             timestamp + newPlaybackDelaySlider, duration);

        newPlaybackSequenceGeneratedAndShouldBeSent = true;
    }

    // MUST Notify What Data Ready to be Sent
    // If you don't want to send anything, just set both flags to false
    // If you have a new playback policy, set the first flag to true
    // If you have a new playback sequence, set the second flag to true
    return {newPlaybackPolicyShouldBeSent, newPlaybackSequenceGeneratedAndShouldBeSent};
}


// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
//                                    DO NOT CHANGE ANYTHING BELOW THIS LINE
// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

using namespace debugging_settings::PlaybackPreparatorThread;

PlaybackPreparatorThread::PlaybackPreparatorThread() : juce::Thread("PlaybackPreparatorThread") {
}

void PlaybackPreparatorThread::startThreadUsingProvidedResources(
        LockFreeQueue<ModelOutput, queue_settings::MDL2PPP_que_size> *MDL2PPP_ModelOutput_Que_ptr_,
        LockFreeQueue<GenerationEvent, queue_settings::PPP2NMP_que_size> *PPP2NMP_GenerationEvent_Que_ptr_,
        LockFreeQueue<GuiParams, queue_settings::APVM_que_size> *APVM2PPP_Parameters_Queu_ptr_) {

    // Provide access to resources needed to communicate with other threads
    // ---------------------------------------------------------------------------------------------
    MDL2PPP_ModelOutput_Que_ptr = MDL2PPP_ModelOutput_Que_ptr_;
    PPP2NMP_GenerationEvent_Que_ptr = PPP2NMP_GenerationEvent_Que_ptr_;
    APVM2PPP_Parameters_Queu_ptr = APVM2PPP_Parameters_Queu_ptr_;

    // Start the thread. This function internally calls run() method. DO NOT CALL run() DIRECTLY.
    // ---------------------------------------------------------------------------------------------
    startThread();
}

void PlaybackPreparatorThread::PrintMessage(const std::string& input) {

    if (disable_user_print_requests) { return; }

    // if input is multiline, split it into lines and print each line separately
    std::stringstream ss(input);
    std::string line;
    while (std::getline(ss, line)) { std::cout << clr::magenta << "[PPP] " << line << clr::reset << std::endl; }
}

void PlaybackPreparatorThread::run() {

    // convert showMessage to a lambda function
    auto showMessage = [](const std::string &input) {
        // if input is multiline, split it into lines and print each line separately
        std::stringstream ss(input);
        std::string line;

        while (std::getline(ss, line)) {
            std::cout << clr::magenta << "[PPP] " << line << clr::reset << std::endl;
        }
    };

    bool new_model_output_received{false};

    // notify if the thread is still running
    bool bExit = threadShouldExit();

    int cnt{0};

    while (!bExit) {

        if (readyToStop) { break; } // check if thread is ready to be stopped

        if (APVM2PPP_Parameters_Queu_ptr->getNumReady() > 0) {
            // print updated values for debugging
            gui_params = APVM2PPP_Parameters_Queu_ptr->pop(); // pop the latest parameters from the queue
            gui_params.registerAccess();                      // set the time when the parameters were accessed

            if (print_received_gui_params) { // if set in Debugging.h
                showMessage(gui_params.getDescriptionOfUpdatedParams());
            }
        } else {
            gui_params.setChanged(false); // no change in parameters since last check
        }

        if (MDL2PPP_ModelOutput_Que_ptr->getNumReady() > 0) {
            new_model_output_received = true;
            model_output = MDL2PPP_ModelOutput_Que_ptr->pop(); // get the earliest queued generation data
            model_output.timer.registerEndTime();

            if (print_ModelOutput_transfer_delay) {
                showMessage(*model_output.timer.getDescription("ModelOutput Transfer Delay from MDL to PPP: "));
            }
        } else {
            new_model_output_received = false;
        }

        if (new_model_output_received or gui_params.changed()) {
            auto status = deploy(new_model_output_received, gui_params.changed());
            auto shouldSendNewPlaybackPolicy = status.first;
            auto shouldSendNewPlaybackSequence = status.second;
            if (shouldSendNewPlaybackPolicy) {
                // send to the the main thread (NMP)
                if (playbackPolicy.IsReadyForTransmission()) {
                    PPP2NMP_GenerationEvent_Que_ptr->push(GenerationEvent(playbackPolicy));
                }
            }

            if (shouldSendNewPlaybackSequence) {
                // send to the the main thread (NMP)
                PPP2NMP_GenerationEvent_Que_ptr->push(GenerationEvent(playbackSequence));
                cnt++;
            }
        }

        // ============================================================================================================

        // check if thread is still running
        bExit = threadShouldExit();
    }

    // wait for a few ms to avoid burning the CPU
    sleep(thread_configurations::PlaybackPreparator::waitTimeBtnIters);
}

void PlaybackPreparatorThread::prepareToStop() {
    // Need to wait enough to ensure the run() method is over before killing thread
    this->stopThread(100 * thread_configurations::PlaybackPreparator::waitTimeBtnIters);

    readyToStop = true;
}

PlaybackPreparatorThread::~PlaybackPreparatorThread() {
    if (not readyToStop) {
        prepareToStop();
    }
}