//
// Created by ANON on 2022-02-11.
//
#pragma once

// #include <utility>

#include "../DeploymentSettings/ThreadsAndQueuesAndInputEvents.h"
#include "../DeploymentSettings/GuiAndParams.h"

#include <torch/script.h> // One-stop header.

#include <utility>


using namespace std;

// ============================================================================================================
// ==========          LockFreeQueue (First In - First Out)          ==========================================
// ============================================================================================================
template<typename T, int queue_size>
class LockFreeQueue {
private:
    //juce::ScopedPointer<juce::AbstractFifo> lockFreeFifo;   depreciated!!
    std::unique_ptr<juce::AbstractFifo> lockFreeFifo;
    juce::Array<std::unique_ptr<T>> data;

    // keep track of number of reads/writes and the latest_value without moving FIFO
    int num_reads = 0;
    int num_writes = 0;
    T latest_written_data{};
    bool writingActive = false;

public:
    LockFreeQueue() {
        // lockFreeFifo = new juce::AbstractFifo(queue_size);   depreciated!!
        lockFreeFifo = std::unique_ptr<juce::AbstractFifo>(
                new juce::AbstractFifo(queue_size));

        // data.ensureStorageAllocated(queue_size);
        while (data.size() < queue_size) {
            // check if T is a tuple
            auto empty_exp = make_shared<T>();
            data.add(nullptr);
        }
    }

    int getNumReady() {
        return lockFreeFifo->getNumReady();
    }

    void push(T writeData) {
        // check if this object is null
        if (this == nullptr) {
            DBG(" [******] You've forgotten to Initialize the LockFreeQueue object! "
                "");
            assert(false && " You've forgotten to Initialize the LockFreeQueue object! "
                            "Double check the processor constructor !!");

        }
        int start1, start2, blockSize1, blockSize2;
        writingActive = true;
        lockFreeFifo->prepareToWrite(
                1, start1, blockSize1,
                start2, blockSize2);
        auto start_data_ptr = data.getRawDataPointer() + start1;
        *start_data_ptr = make_unique<T>(writeData);
        latest_written_data = writeData;
        num_writes += 1;
        lockFreeFifo->finishedWrite(1);
        writingActive = false;
    }

    T pop() {
        int start1, start2, blockSize1, blockSize2;

        lockFreeFifo->prepareToRead(
                1, start1, blockSize1,
                start2, blockSize2);

        auto start_data_ptr = data.getRawDataPointer() + start1;

        /* This way, when the returned value goes out of scope, it will be automatically deleted. Note that you need to
         * std::move the unique pointer, so the ownership of the dynamically allocated data is transferred to the
         * returned value.*/
        auto res = std::move(*(*(start_data_ptr)));
        lockFreeFifo->finishedRead(1);
        num_reads += 1;

        return res;
    }


    T getLatestOnly() {
        int start1, start2, blockSize1, blockSize2;
        T readData;

        lockFreeFifo ->prepareToRead(
                getNumReady(), start1, blockSize1,
                start2, blockSize2);

        if (blockSize2 > 0) {
            auto start_data_ptr = data.getRawDataPointer() + start2;
            readData = *(*(start_data_ptr+blockSize2-1));
            lockFreeFifo -> finishedRead(blockSize1+blockSize2);
            num_reads += 1;
            return readData;

        }
        if (blockSize1 > 0) {
            auto start_data_ptr = data.getRawDataPointer() + start1;
            readData = *(*(start_data_ptr+blockSize1-1));
            lockFreeFifo -> finishedRead(blockSize1+blockSize2);
            num_reads += 1;
            return readData;
        }

    }

    int getNumberOfReads() {
        return num_reads;
    }

    int getNumberOfWrites() {
        return num_writes;
    }

    // This method is useful for keeping track of whether any data has previously
    //      written to Queue regardless of being read or not
    // !! This method should only be used for initialization of GUI objects !!
    // !!! To use the QUEUE for lock free communication use the ReadFrom() or pop() methods!!!
    T getLatestDataWithoutMovingFIFOHeads() {
        return latest_written_data;
    }

    bool isWritingInProgress() {
        return writingActive;
    }
};
