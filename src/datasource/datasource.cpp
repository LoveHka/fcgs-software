#include <cmath>
#include "datasource.h"

double DataSource::next()
{
    time += 0.016;
    return std::sin(time);
}

