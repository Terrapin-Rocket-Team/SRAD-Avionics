#ifndef FIR_FILTER_H
#define FIR_FILTER_H

#include <Filters/Filter.h>
#include <Utils/CircBuffer.h>

namespace mmfs
{
    class FIRFilter : public Filter
    {
    public:
        FIRFilter(int numTaps, double *coefficients);
        ~FIRFilter() override;
        void initialize() override;

        // only one input (measurement) and one output (state), so dt is optional and controlVars can be a random pointer (it's not used)
        double* iterate(double dt, double* state, double* measurements, double* controlVars) override;
        
        // Provide interface to query filter dimensions
        int getMeasurementSize() const override;
        int getInputSize() const override { return 0; };    // no control variables
        int getStateSize() const override;

    private:
        
        //-------------------Implement Code Here-------------------//
        
        // Instance variables to store filter state & circular buffer

    };
}


#endif // FIR_FILTER_H