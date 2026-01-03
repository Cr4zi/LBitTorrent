#pragma once

class Connection {
public:
    virtual ~Connection() = default;

    virtual void OnWrite() = 0;
    virtual void OnRead() = 0;
    virtual void OnError() = 0;
};
