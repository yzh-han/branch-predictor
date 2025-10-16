

enum State {
     STRONGLY_NOT_TAKEN = 0, 
     WEAKLY_NOT_TAKEN = 1, 
     WEAKLY_TAKEN = 2, 
     STRONGLY_TAKEN = 3
};

void updateCounterState(bool taken, State& currentState) {
    if (taken) {
        // If branch was taken, increment the counter (if not already at max)
        if (currentState == WEAKLY_NOT_TAKEN) currentState = STRONGLY_TAKEN;
        else if (currentState < STRONGLY_TAKEN) currentState = static_cast<State>(currentState + 1);
    } 
    else {
        // If branch was not taken, decrement the counter (if not already at min)
        if (currentState == WEAKLY_TAKEN) currentState = STRONGLY_NOT_TAKEN;
        else if (currentState > STRONGLY_NOT_TAKEN) currentState = static_cast<State>(currentState - 1);
    }
}
