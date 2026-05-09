#include <cstring>
#include "point.h"
#include "common.h"

struct TDeque {
    Point data[MAX_POINTS];
    int head = 0;
    int tail = 0;

    inline void push_back(Point p) { 
        data[tail++] = p; 
    }
    
    inline Point front() { 
        return data[head]; 
    }
    
    inline void pop_front() { 
        head++; 
    }
    
    inline bool empty() { 
        return head == tail; 
    }
    
    inline void clear() { 
        head = 0; 
        tail = 0; 
    }
    
    inline int size() { 
        return tail - head; 
    }

    inline Point get(int i) {
        return data[i];
    }
};