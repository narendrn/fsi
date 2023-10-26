#ifndef FASTMUTEX_H
#define FASTMUTEX_H

class FastMutex
{
public:
    FastMutex();
    void Lock();
    void Unlock();
private:
    int locked;
};

#endif // FASTMUTEX_H
