#include <cmath>
#include "datasource.h"

double DataSource::next()
{
    time += 0.016;
    return 5 + std::sin(time);
}

