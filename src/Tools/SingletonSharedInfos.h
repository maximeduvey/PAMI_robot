#pragma once

/// this file is just a lazy "singleton", to share static infos between classes
/// should be refactored

class SingletonSharedInfos {
public:
    // Get the instance of the singleton
    static SingletonSharedInfos& getInstance() {
        static SingletonSharedInfos instance;
        return instance;
    }

    // Accessor and mutator for the shared variable
    int getPAMI_ID() const { return PAMI_ID; }
    void setPAMI_ID(int id) { PAMI_ID = id; }

private:
    int PAMI_ID = 0;  // Shared variable

    // Private constructor, copy constructor, and assignment operator to enforce singleton pattern
    SingletonSharedInfos() = default;
    SingletonSharedInfos(const SingletonSharedInfos&) = delete;
    SingletonSharedInfos& operator=(const SingletonSharedInfos&) = delete;
};


