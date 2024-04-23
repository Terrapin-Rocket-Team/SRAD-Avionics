classdef MultiVarKalmanFilter6
    properties
        F % State Transition Matrix
        G % Control Matrix
        U % Control Vector
        X % State Vector
        H % Observation Matrix
        P % Estimate Covariance Matrix
        R % Measurement Uncertainty Matrix
        K % Kalman Gain
        Q % Process Noise Matrix
    end
    
    methods
        % Constructor
        function obj = MultiVarKalmanFilter6(U, X, P)
            obj.U = U';
            obj.X = X';
            obj.P = P;
        end

        function obj = predictState(obj)
            obj.X = obj.F * obj.X + obj.G * obj.U;
        end
        
        function obj = estimateState(obj, measurement)
            obj.X = obj.X + obj.K*(measurement - obj.H*obj.X);
        end

        function obj = calculateKalmanGain(obj)
            obj.K = obj.P * obj.H' * inv((obj.H * obj.P * obj.H' + obj.R));
        end

        function obj = covarianceUpdate(obj)
            n = 6;
            obj.P = (eye(n) - obj.K*obj.H)*obj.P*(eye(n) - obj.K*obj.H)' + obj.K*obj.R*obj.K';
        end

        function obj = covarianceExtrapolate(obj)
            obj.P = obj.F*obj.P*obj.F'+ obj.Q;
        end
        
        function obj = calculateInitialValues(obj, dt)
            obj.F =    [1, 0, 0, dt, 0, 0;
                        0, 1, 0, 0, dt, 0;
                        0, 0, 1, 0, 0, dt;
                        0, 0, 0, 1, 0,  0;
                        0, 0, 0, 0, 1,  0;
                        0, 0, 0, 0, 0,  1];

            obj.G =    [0.5*dt*dt, 0, 0;
                        0, 0.5*dt*dt, 0;
                        0, 0, 0.5*dt*dt;
                        dt, 0,        0;
                        0, dt,        0;
                        0, 0,       dt];
            
            obj.Q = obj.G*0.2*0.2*obj.G';

            obj = predictState(obj);
            obj = covarianceExtrapolate(obj);
        end

        function obj = iterate(obj, dt, measurement, control)
            % Update step
            z = measurement;
            u = control;

            obj.F =    [1, 0, 0, dt, 0, 0;
                        0, 1, 0, 0, dt, 0;
                        0, 0, 1, 0, 0, dt;
                        0, 0, 0, 1, 0,  0;
                        0, 0, 0, 0, 1,  0;
                        0, 0, 0, 0, 0,  1];

            obj.G =    [0.5*dt*dt, 0, 0;
                        0, 0.5*dt*dt, 0;
                        0, 0, 0.5*dt*dt;
                        dt, 0,        0;
                        0, dt,        0;
                        0, 0,       dt];

            obj.Q = obj.G*1.5*1.5*obj.G';

            obj.R = [1.5, 0, 0, 0;
                     0, 1.5, 0, 0;
                     0, 0, 1.5, 1.5;
                     0, 0, 1.5, 1.5; ];

            obj.H = [1, 0, 0, 0, 0, 0;
                     0, 1, 0, 0, 0, 0;
                     0, 0, 1, 0, 0, 0;
                     0, 0, 1, 0, 0, 0];


            obj = calculateKalmanGain(obj);
            obj = estimateState(obj, measurement');
            obj = covarianceUpdate(obj);

            % Predict step
            obj = predictState(obj);
            obj = covarianceExtrapolate(obj);
        end
    end
end


