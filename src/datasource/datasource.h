#pragma once


class DataSource
{
    public:
        double next();
    
    private:
        double time = 0.0;
};
