function motor_identification_app(data)
%MOTOR_IDENTIFICATION_APP  Identification module of the DC Motor Control Trainer.
%
%   motor_identification_app        opens the module and lets the user load
%                                   a data file saved by the GUI.
%   motor_identification_app(D)     opens the module with the data matrix D
%                                   already loaded (one row per sample).
%
%   Models:
%     1) First order + dead time:  G(s) = K e^(-theta s) / (tau s + 1)
%     2) Second order:             G(s) = K / ((tau1 s + 1)(tau2 s + 1))
%     3) State space, grey box:    x = [i; w], parameters R, L, Ke=Kt, B, J
%
%   Estimation: nonlinear least squares (Optimization Toolbox + Control
%   System Toolbox). If the System Identification Toolbox is installed, the
%   option "Use tfest/ssest" estimates models 1-2 with tfest and a black-box
%   state-space model with ssest (simulation focus, no noise model).
%
%   Dead time: the platform has about one sample of delay. It is estimated
%   in whole samples with the first-order model and used by all three models
%   (row "theta" of the results table).
%
%   Default columns (telemetry order of the ESP32 firmware):
%     1 current [mA] | 2 speed [rad/s] | 3 time [s] | 4 voltage [V]
%   Change them in the "Columns" fields if the saved file uses another order.
%   Files saved with the Save button of the GUI (table "data") are detected
%   and reordered automatically.
%
%   Part of: An IoT-Enabled Pedagogical Platform for Teaching Automatic
%   Control with a DC Motor (ITESO).

    n_gear = 44.4;                  % gearbox ratio (measured; same as GEAR_BOX in the firmware)
    S = struct('raw',[],'val',[],'model',[],'res',[]);
    hasSIT = ~isempty(which('tfest')) && license('test','Identification_Toolbox');

    busy = false;                                  % true while a file dialog is open

    % ------------------------------------------------------------- layout
    % Window size adapted to the screen and centered on it, wide enough for
    % three columns: controls, plots and the results table.
    scr  = get(groot,'ScreenSize');
    figW = min(1500, round(0.94*scr(3)));
    figH = min(900,  round(0.88*scr(4)));
    figX = max(1, round((scr(3) - figW)/2));
    figY = max(1, round((scr(4) - figH)/2));
    fig = uifigure('Name','Model Identification - DC Motor Trainer', ...
                   'Position',[figX figY figW figH]);
    g = uigridlayout(fig,[1 3]); g.ColumnWidth = {340,'1x',390};

    left = uigridlayout(g,[22 2]); left.RowHeight = repmat({24},1,22);
    left.Layout.Row = 1; left.Layout.Column = 1;
    left.ColumnWidth = {150,'1x'}; left.Padding = [8 8 8 8];

    r = 1;
    lbl(left,r,1,'1. Data','bold');                                  r=r+1;
    b = uibutton(left,'Text','Load identification data...', ...
        'ButtonPushedFcn',@(~,~) onLoad(false));
    b.Layout.Row = r; b.Layout.Column = [1 2];                       r=r+1;
    fileLbl = lbl(left,r,[1 2],'No data loaded','normal');           r=r+1;
    lbl(left,r,1,'Columns i, w, t, V');
    colEdit = uieditfield(left,'text','Value','1 2 3 4');
    colEdit.Layout.Row = r; colEdit.Layout.Column = 2;               r=r+1;
    lbl(left,r,1,'Segment start [s]');
    t0Edit = uieditfield(left,'numeric','Value',0);
    t0Edit.Layout.Row = r; t0Edit.Layout.Column = 2;                 r=r+1;
    lbl(left,r,1,'Segment end [s] (0 = all)');
    t1Edit = uieditfield(left,'numeric','Value',0);
    t1Edit.Layout.Row = r; t1Edit.Layout.Column = 2;                 r=r+1;

    lbl(left,r,1,'2. Model','bold');                                 r=r+1;
    modelDD = uidropdown(left,'Items', ...
        {'First order + dead time','Second order','State space (grey box)'});
    modelDD.Layout.Row = r; modelDD.Layout.Column = [1 2];           r=r+1;
    sitCB = uicheckbox(left,'Text','Use tfest/ssest (System Identification Toolbox)', ...
        'Value',false,'Enable',matlab.lang.OnOffSwitchState(hasSIT));
    sitCB.Layout.Row = r; sitCB.Layout.Column = [1 2];               r=r+1;
    b = uibutton(left,'Text','Estimate model','FontWeight','bold', ...
        'ButtonPushedFcn',@(~,~) onEstimate());
    b.Layout.Row = r; b.Layout.Column = [1 2];                       r=r+1;

    lbl(left,r,1,'3. Validation','bold');                            r=r+1;
    b = uibutton(left,'Text','Load validation data and compare...', ...
        'ButtonPushedFcn',@(~,~) onLoad(true));
    b.Layout.Row = r; b.Layout.Column = [1 2];                       r=r+1;

    lbl(left,r,1,'4. PI design (lambda)','bold');                    r=r+1;
    lbl(left,r,1,'lambda [s]');
    lamEdit = uieditfield(left,'numeric','Value',0.3,'Limits',[0.01 10]);
    lamEdit.Layout.Row = r; lamEdit.Layout.Column = 2;               r=r+1;
    b = uibutton(left,'Text','Compute PI gains', ...
        'ButtonPushedFcn',@(~,~) onPI());
    b.Layout.Row = r; b.Layout.Column = [1 2];                       r=r+1;

    b = uibutton(left,'Text','Export model to workspace', ...
        'ButtonPushedFcn',@(~,~) onExport());
    b.Layout.Row = r; b.Layout.Column = [1 2];                       r=r+1;

    % The controls keep their fixed height at the top of the left column.
    left.RowHeight = [repmat({24},1,r-1), {'1x'}];

    % Plots in the middle column
    right = uigridlayout(g,[2 1]);
    right.Layout.Row = 1; right.Layout.Column = 2;

    % Results table in its own column, using the full height of the window,
    % so all its rows (model, validation and PI design) are visible.
    tbl = uitable(g,'ColumnName',{'Quantity','Value','Unit'}, ...
        'ColumnWidth',{170,'auto',80},'RowName',{});
    tbl.Layout.Row = 1; tbl.Layout.Column = 3;

    ax1 = uiaxes(right); title(ax1,'Speed'); xlabel(ax1,'Time (s)'); ylabel(ax1,'\omega (rad/s)'); grid(ax1,'on');
    ax2 = uiaxes(right); title(ax2,'Voltage and current'); xlabel(ax2,'Time (s)'); grid(ax2,'on');

    if nargin >= 1 && ~isempty(data)
        S.raw = data; fileLbl.Text = 'Data passed from the GUI'; plotData();
    end

    % ========================================================= callbacks
    function onLoad(isValidation)
        % Ignore repeated clicks while the file dialog is open (on macOS a
        % second click could open the dialog again).
        if busy, return; end
        busy = true;
        [f,p] = uigetfile({'*.mat;*.csv;*.txt','Data files (*.mat, *.csv, *.txt)'}, ...
                          'Select data file');
        figure(fig);                               % bring the module to the front
        busy = false;
        if isequal(f,0), return; end
        try
            D = readDataFile(fullfile(p,f));
        catch err
            uialert(fig,err.message,'Could not read the file'); return;
        end
        if isValidation
            if isempty(S.model)
                uialert(fig,'Estimate a model first.','Validation'); return;
            end
            S.val = D;
            try
                validate(f);
            catch err
                uialert(fig,err.message,'Validation failed');
            end
        else
            % A new file clears the results of the previous one
            S.raw = D; S.model = []; fileLbl.Text = f; tbl.Data = {}; plotData();
        end
    end

    function onEstimate()
        if isempty(S.raw), uialert(fig,'Load data first.','Estimate'); return; end
        try
            d = prepare(S.raw);
            switch modelDD.Value
                case 'First order + dead time', S.model = estFO(d);
                case 'Second order',            S.model = estSO(d);
                otherwise,                      S.model = estSS(d);
            end
        catch err
            uialert(fig,err.message,'Estimation failed'); return;
        end
        showModel(d);
    end

    function onPI()
        if isempty(S.model), uialert(fig,'Estimate a model first.','PI design'); return; end
        [K,tau] = firstOrderEquivalent(S.model);
        lam = lamEdit.Value;
        Kp = tau/(K*lam); Ki = 1/(K*lam);
        addRows({'--- PI design ---','','';
                 'K_P', fmt(Kp), 'V s/rad';
                 'K_I', fmt(Ki), 'V/rad';
                 'Settling time ~4 lambda', fmt(4*lam), 's';
                 'V at step of 8 rad/s (K_P r)', fmt(8*Kp), 'V'});
    end

    function onExport()
        if isempty(S.model), uialert(fig,'Estimate a model first.','Export'); return; end
        assignin('base','identifiedModel',S.model);
        uialert(fig,'Variable "identifiedModel" created in the base workspace.', ...
                'Export','Icon','success');
    end

    % ===================================================== data handling
    function D = readDataFile(fname)
        [~,~,ext] = fileparts(fname);
        if strcmpi(ext,'.mat')
            V = load(fname); fn = fieldnames(V); D = [];
            for k = 1:numel(fn)
                x = V.(fn{k});
                if istable(x)
                    D = guiTableToTelemetry(x);   % file saved with the GUI Save button
                    if ~isempty(D), break; end
                    x = table2array(x);
                end
                if isnumeric(x) && ismatrix(x) && size(x,2) >= 4 && size(x,1) > 10
                    D = double(x); break;
                end
            end
            if isempty(D)
                error('No numeric matrix with at least 4 columns was found in the MAT file.');
            end
        else
            D = readmatrix(fname);
        end
        D = D(all(isfinite(D(:,1:min(4,end))),2),:);
    end

    function D = guiTableToTelemetry(T)
        % The GUI saves the table "data" with the columns Time, Speed,
        % FiltSpeed, Current, FiltCurrent, Voltage, Error, DutyCycle.
        % It is reordered to the default column order i, w, t, V.
        D = [];
        need = {'Current','Speed','Time','Voltage'};
        if all(ismember(need, T.Properties.VariableNames))
            D = double([T.Current(:), T.Speed(:), T.Time(:), T.Voltage(:)]);
            colEdit.Value = '1 2 3 4';
        end
    end

    function d = prepare(D)
        c = str2num(colEdit.Value); %#ok<ST2NM>
        if numel(c) ~= 4, error('Enter four column numbers: i w t V.'); end
        i = D(:,c(1))/1000;  w = D(:,c(2));  t = D(:,c(3));  v = D(:,c(4));
        t = t - t(1);
        [t,ia] = unique(t); i = i(ia); w = w(ia); v = v(ia);
        sel = t >= t0Edit.Value;
        if t1Edit.Value > 0, sel = sel & t <= t1Edit.Value; end
        t = t(sel); i = i(sel); w = w(sel); v = v(sel);
        if numel(t) < 20, error('The selected segment has too few samples.'); end
        Ts = median(diff(t));                       % uniform resampling for lsim
        tu = (t(1):Ts:t(end))';
        d.t  = tu - tu(1);
        % 'nearest' (not 'previous'): the time stamps of the ESP32 jitter by a few
        % microseconds around the uniform grid; with 'previous', a sample stamped
        % 2 us after its grid point is assigned to the next one and the step moves
        % one sample, so the result depended on the sign of that jitter. The real
        % delay of the platform is now estimated explicitly as theta (estFO).
        d.v  = interp1(t,v,tu,'nearest');
        d.w  = interp1(t,w,tu,'linear');
        d.i  = interp1(t,i,tu,'linear');
        d.Ts = Ts;
        n0 = max(1,min(5,round(0.05*numel(tu))));   % initial operating point
        % Operating point. If the segment starts with the motor at rest, the
        % absolute signals are used (the voltage can be applied from the first
        % sample); otherwise, the deviations from the initial steady state.
        % (The first sample is used: in closed loop the motor can start to
        % turn within the first few samples.)
        d.fromRest = abs(d.w(1)) < 0.1;
        if d.fromRest
            d.v0 = 0; d.w0 = 0; d.i0 = 0;
        else
            d.v0 = mean(d.v(1:n0)); d.w0 = mean(d.w(1:n0)); d.i0 = mean(d.i(1:n0));
        end
        if max(d.v) - min(d.v) < 0.5
            error(['The voltage does not change enough in the selected segment ' ...
                   '(%.2f V). Use a Step experiment, or move "Segment start" ' ...
                   'before the change of voltage (for example, 0 s).'], max(d.v) - min(d.v));
        end
    end

    % ========================================================= estimation
    function m = estFO(d)
        u = d.v - d.v0;  y = d.w - d.w0;
        if sitCB.Value && hasSIT
            z = iddata(y,u,d.Ts);
            sys = tfest(z,1,0,NaN);
            K = dcgain(sys); p = pole(sys); tau = -1/real(p(1)); theta = sys.IODelay;
        else
            [K,tau,theta] = fitFOdelay(d,u,y);
        end
        if theta < 1e-6, theta = 0; end            % numerically zero dead time
        m = struct('type','FO','K',K,'tau',tau,'theta',theta, ...
                   'G',tf(K,[tau 1],'InputDelay',theta),'Ts',d.Ts,'n',n_gear);
    end

    function m = estSO(d)
        u = d.v - d.v0;  y = d.w - d.w0;
        [K0,tau0,theta] = fitFOdelay(d,u,y);        % dead time of the platform
        if sitCB.Value && hasSIT
            z = iddata(y,u,d.Ts);
            sys = tfest(z,2,0,theta);
            G = tf(sys); K = dcgain(G); p = sort(-1./real(pole(G)),'descend');
            tau1 = p(1); tau2 = p(2);
        else
            fun = @(q) lsim(tf(q(1),conv([q(2) 1],[q(3) 1]),'InputDelay',theta),u,d.t) - y;
            q = fitLS(fun,[K0 tau0 0.1*tau0],[0 1e-4 1e-4],[Inf 10 10]);
            K = q(1); tau1 = max(q(2),q(3)); tau2 = min(q(2),q(3));
        end
        m = struct('type','SO','K',K,'tau1',tau1,'tau2',tau2,'theta',theta, ...
                   'G',tf(K,conv([tau1 1],[tau2 1]),'InputDelay',theta), ...
                   'Ts',d.Ts,'n',n_gear);
    end

    function m = estSS(d)
        u  = d.v - d.v0;
        Y  = [d.i - d.i0, d.w - d.w0];
        [K0,tau0,theta] = fitFOdelay(d,u,Y(:,2));  % dead time of the platform
        if sitCB.Value && hasSIT                      % black-box model, order 2
            % Simulation focus, zero initial state and no noise model: with the
            % default options ssest fits the one-step-ahead prediction and the
            % simulated response can be poor (undershoot, wrong current level).
            z = iddata(Y,u,d.Ts);
            opt = ssestOptions('Focus','simulation','InitialState','zero');
            sys = ssest(z,2,'Feedthrough',false,'DisturbanceModel','none', ...
                        'InputDelay',theta,opt);
            m = struct('type','SSB','sys',ss(sys),'theta',theta,'Ts',d.Ts,'n',n_gear);
            return;
        end
        % Grey box: parameters q = log([R L Ke B J]) for positivity and scaling
        R0 = 10; Ke0 = 1/(n_gear*K0); B0 = 0.05*Ke0^2/R0;
        J0 = tau0*(R0*B0 + Ke0^2)/R0; L0 = 1e-3;
        q0 = log([R0 L0 Ke0 B0 J0]);
        sc = std(Y); sc(sc == 0) = 1;
        fun = @(q) reshape((simSS(exp(q),theta,d.t,u) - Y)./sc,[],1);
        q = fitLS(fun,q0,q0-8,q0+8);
        P = exp(q);
        m = struct('type','SS','R',P(1),'L',P(2),'Ke',P(3),'B',P(4),'J',P(5), ...
                   'theta',theta,'sys',ssModel(P,theta),'Ts',d.Ts,'n',n_gear);
    end

    function q = fitLS(fun,q0,lb,ub)
        if exist('lsqnonlin','file') == 2
            opt = optimoptions('lsqnonlin','Display','off', ...
                'MaxFunctionEvaluations',4000,'MaxIterations',400);
            q = lsqnonlin(fun,q0,lb,ub,opt);
        else                                          % fallback without toolbox
            cost = @(x) sum(fun(min(max(x,lb),ub)).^2);
            q = fminsearch(cost,q0,optimset('Display','off','MaxFunEvals',8000));
            q = min(max(q,lb),ub);
        end
    end

    function [K,tau,theta] = fitFOdelay(d,u,y)
        % First order + dead time with the dead time searched in whole samples
        % (0 to 5 Ts); K and tau are fitted for each one and the best fit is
        % kept. Estimating theta as a free continuous parameter does not work:
        % below one sample lsim hardly changes with theta, so the optimizer
        % stays at its initial value 0. The platform has about one sample of
        % delay (the voltage written at sample k is seen in the speed from
        % sample k+2). The same theta is used by the other two models.
        [K0,tau0] = initialFO(d.t,u,y);
        best = Inf; K = K0; tau = tau0; theta = 0;
        for nd = 0:5
            th  = nd*d.Ts;
            fun = @(q) simFO([q(1) q(2) th],d.t,u) - y;
            q   = fitLS(fun,[K0 tau0],[0 1e-3],[Inf 10]);
            sse = sum(fun(q).^2);
            if sse < best
                best = sse; K = q(1); tau = q(2); theta = th;
            end
        end
    end

    function [K0,tau0] = initialFO(t,u,y)
        nEnd = max(1,round(0.1*numel(t)));
        du = mean(u(end-nEnd+1:end)); dy = mean(y(end-nEnd+1:end));
        if abs(du) < 1e-6, du = max(abs(u)); end
        K0 = max(abs(dy/du),1e-3);
        kStep = find(abs(u) > 0.5*abs(du),1);
        k63 = find(abs(y) >= 0.632*abs(dy),1);
        if isempty(kStep) || isempty(k63) || k63 <= kStep, tau0 = 0.1;
        else, tau0 = max(t(k63) - t(kStep),1e-2); end
    end

    function y = simFO(q,t,u)
        y = lsim(tf(q(1),[q(2) 1],'InputDelay',q(3)),u,t);
    end

    function sys = ssModel(P,theta)
        R = P(1); L = P(2); Ke = P(3); B = P(4); J = P(5);
        A  = [-R/L, -n_gear*Ke/L; Ke/(n_gear*J), -B/J];
        Bm = [1/L; 0];
        sys = ss(A,Bm,eye(2),zeros(2,1),'InputDelay',theta, ...
                 'StateName',{'i','w'},'OutputName',{'i (A)','w (rad/s)'});
    end

    function Y = simSS(P,theta,t,u)
        Y = lsim(ssModel(P,theta),u,t);
    end

    % ============================================ results and validation
    function showModel(d)
        [yhat,ihat] = simulate(S.model,d);
        fw = fitPercent(d.w - d.w0,yhat);
        rows = {'Model', S.model.type, '';
                'Sampling period', fmt(d.Ts), 's';
                'Samples', sprintf('%d',numel(d.t)), ''};
        if d.fromRest
            rows = [rows; {'Operating point','from rest (absolute)',''}];
        else
            rows = [rows; {'Operating point', sprintf('v0 = %.2f V, w0 = %.2f rad/s',d.v0,d.w0), ''}];
        end
        switch S.model.type
            case 'FO'
                rows = [rows; {'K',fmt(S.model.K),'rad/(V s)'; 'tau',fmt(S.model.tau),'s';
                               'theta',fmt(S.model.theta),'s'}];
            case 'SO'
                rows = [rows; {'K',fmt(S.model.K),'rad/(V s)'; 'tau1',fmt(S.model.tau1),'s';
                               'tau2',fmt(S.model.tau2),'s'; 'theta',fmt(S.model.theta),'s'}];
            case 'SS'
                [K,tau] = firstOrderEquivalent(S.model);
                rows = [rows; {'R',fmt(S.model.R),'Ohm'; 'L',fmt(S.model.L),'H';
                               'Ke = Kt',fmt(S.model.Ke),'V s/rad'; 'B',fmt(S.model.B),'N m s';
                               'J',fmt(S.model.J),'kg m^2'; 'tau_e = L/R',fmt(S.model.L/S.model.R),'s';
                               'Equivalent K',fmt(K),'rad/(V s)'; 'Equivalent tau',fmt(tau),'s';
                               'theta',fmt(S.model.theta),'s'}];
                if S.model.L/S.model.R < d.Ts
                    % tau_e is below the sampling period: the electrical mode is
                    % not observable in the data and L (hence tau_e) is not
                    % reliable. It is measured with an oscilloscope (Practice 3.2).
                    rows = [rows; {'Note','L not reliable: tau_e < Ts',''}];
                end
            case 'SSB'
                [K,tau] = firstOrderEquivalent(S.model);
                rows = [rows; {'Equivalent K',fmt(K),'rad/(V s)'; 'Equivalent tau',fmt(tau),'s';
                               'theta',fmt(S.model.theta),'s'}];
        end
        rows = [rows; {'Fit speed (estimation)', sprintf('%.1f',fw), '%'}];
        if ~isempty(ihat)
            rows = [rows; {'Fit current (estimation)', sprintf('%.1f',fitPercent(d.i - d.i0,ihat)), '%'}];
        end
        tbl.Data = rows;
        plotFit(d,yhat,ihat,'Estimation data');
    end

    function validate(fname)
        d = prepare(S.val);
        [yhat,ihat] = simulate(S.model,d);
        rows = {'--- Validation ---', fname, '';
                'Fit speed (validation)', sprintf('%.1f',fitPercent(d.w - d.w0,yhat)), '%'};
        if ~isempty(ihat)
            rows = [rows; {'Fit current (validation)', sprintf('%.1f',fitPercent(d.i - d.i0,ihat)), '%'}];
        end
        addRows(rows);
        plotFit(d,yhat,ihat,'Validation data');
    end

    function [yw,yi] = simulate(m,d)
        u = d.v - d.v0; yi = [];
        switch m.type
            case {'FO','SO'}
                yw = lsim(m.G,u,d.t);
            otherwise
                Y = lsim(m.sys,u,d.t); yi = Y(:,1); yw = Y(:,2);
        end
    end

    function [K,tau] = firstOrderEquivalent(m)
        switch m.type
            case 'FO', K = m.K; tau = m.tau;
            case 'SO', K = m.K; tau = m.tau1 + m.tau2;
            otherwise
                G = tf(m.sys); G = G(2,1); K = dcgain(G);
                p = pole(G); tau = sum(-1./real(p));      % sum of time constants
        end
    end

    function f = fitPercent(y,yhat)
        f = 100*(1 - norm(y - yhat)/norm(y - mean(y)));
    end

    function plotFit(d,yw,yi,ttl)
        cla(ax1); ylim(ax1,'auto'); plot(ax1,d.t,d.w,'.','MarkerSize',6); hold(ax1,'on');
        plot(ax1,d.t,yw + d.w0,'LineWidth',1.5); hold(ax1,'off');
        legend(ax1,{'Measured','Model'},'Location','southeast'); title(ax1,['Speed - ' ttl]);
        clearDual(); yyaxis(ax2,'left'); plot(ax2,d.t,d.v,'LineWidth',1.2); ylabel(ax2,'V (V)');
        setVoltageLimits(d.v);
        yyaxis(ax2,'right'); plot(ax2,d.t,1000*d.i,'.','MarkerSize',5); hold(ax2,'on');
        if ~isempty(yi), plot(ax2,d.t,1000*(yi + d.i0),'-','LineWidth',1.2); end
        ylabel(ax2,'i (mA)'); title(ax2,['Voltage and current - ' ttl]);
        drawnow limitrate;
    end

    function plotData()
        try, d = prepare(S.raw); catch, return; end
        cla(ax1); ylim(ax1,'auto'); plot(ax1,d.t,d.w,'.','MarkerSize',6);
        legend(ax1,{'Measured'},'Location','southeast'); title(ax1,'Speed - loaded data');
        clearDual(); yyaxis(ax2,'left'); plot(ax2,d.t,d.v,'LineWidth',1.2); ylabel(ax2,'V (V)');
        setVoltageLimits(d.v);
        yyaxis(ax2,'right'); plot(ax2,d.t,1000*d.i,'.','MarkerSize',5); ylabel(ax2,'i (mA)');
        title(ax2,'Voltage and current - loaded data'); drawnow limitrate;
    end

    % ============================================================ helpers
    function clearDual()
        % cla does not reset the axis limits; without 'auto' the left axis
        % could keep its initial 0-1 V range and hide the voltage.
        % Both sides are cleared and left in 'hold on': with 'replace', the
        % first plot on the right side after the axes were created could
        % reset the left side and the voltage line was not shown.
        yyaxis(ax2,'right'); cla(ax2); ylim(ax2,'auto'); hold(ax2,'on');
        yyaxis(ax2,'left');  cla(ax2); ylim(ax2,'auto'); hold(ax2,'on');
    end
    function setVoltageLimits(v)
        % Explicit limits for the voltage axis. On the first plot after the
        % module opens, the left axis of yyaxis could keep its initial 0-1 V
        % range even in 'auto' mode, which hid the voltage line.
        lo = min(v); hi = max(v);
        pad = max(0.05*(hi - lo), 0.5);
        ylim(ax2,[lo - pad, hi + pad]);
    end
    function addRows(rows)
        if isempty(tbl.Data), tbl.Data = rows; else, tbl.Data = [tbl.Data; rows]; end
    end
    function s = fmt(x), s = sprintf('%.4g',x); end
    function h = lbl(parent,row,col,txt,weight)
        if nargin < 5, weight = 'normal'; end
        h = uilabel(parent,'Text',txt,'FontWeight',weight);
        h.Layout.Row = row; h.Layout.Column = col;
    end
end
