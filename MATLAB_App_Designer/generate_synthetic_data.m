%% =====================================================================
%  GENERATE SYNTHETIC DATA TO VERIFY THE IDENTIFICATION MODULE
%  (Table 5.5 of the thesis)
%
%  Known model:  G(s) = 1.08 / (0.16 s + 1),  no dead time
%  Input:        3 V from t = 0 and 9 V from t = 2 s
%  Sampling:     10 ms during 6 s
%  Noise:        0.15 rad/s on the speed and 20 mA on the current
%
%  The file is saved with the same table format as the Save button of the
%  GUI, so it can be loaded directly with "Load identification data...".
%% =====================================================================
clear; clc;

Ts = 0.01;                       % sampling period [s]
t  = (0:Ts:6)';                  % time [s]
v  = 3*ones(size(t));            % voltage [V]
v(t >= 2) = 9;

G = tf(1.08, [0.16 1]);          % true model (speed / voltage)
w = lsim(G, v, t);               % noise-free speed [rad/s]

% Armature current from the physical model of the motor (only to fill the
% current column; it is not used by the first-order identification)
R = 14.8; k = 0.0163; n = 44.4;
i = 1000*(v - k*n*w)/R;          % [mA]

rng(1);                          % fixed seed: same data every time
w = w + 0.15*randn(size(t));     % speed noise [rad/s]
i = i + 20*randn(size(t));       % current noise [mA]

% Same columns as the files saved by the GUI
data = table(t, w, w, i, i, v, zeros(size(t)), round(100*v/12), ...
    'VariableNames', {'Time','Speed','FiltSpeed','Current','FiltCurrent', ...
                      'Voltage','Error','DutyCycle'});
save('Synthetic_FO_test.mat', 'data');

fprintf('Synthetic_FO_test.mat created (%d samples).\n', numel(t));
fprintf('True model: K = 1.08 rad/(s V), tau = 0.16 s, theta = 0.\n');
