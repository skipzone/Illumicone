%% Initialization and configuration

clear;

format long;

plotConfig = {'Burning Man 2016', ...   % description for plot title
              '2016-08-26 13:00', ...   % start date and time
              '2016-09-05 12:59:59' ... % end date and time
              -1                        % event timezone hour offset from data timezone
             };

widgetNames = {' '; 'Eye'; 'Shirleys Web'; 'Bells'; 'Steps'; ' '; ...
               'TriObelisk'; ' '; 'Plunger'; 'FourPlay'; ' '};

              
%% Get the widget minute-by-minute activity data from the database.

sql = [ ...
    'SELECT widget_id, minute_active' ...
    ' FROM widget_minute_active' ...
    ' WHERE minute_active BETWEEN ''', cell2mat(plotConfig(2)), '''' ...
    '   AND ''', cell2mat(plotConfig(3)), ''''];

%sql = 'SELECT widget_id, minute_active, UNIX_TIMESTAMP(minute_active) FROM widget_minute_active';
%sql = 'SELECT widget_id, minute_active, UNIX_TIMESTAMP(minute_active) FROM widget_minute_active WHERE minute_active BETWEEN ''2016-08-26 18:00:00'' AND ''2016-09-04 18:00:00''';
%sql = 'SELECT widget_id, minute_active, UNIX_TIMESTAMP(minute_active) FROM widget_minute_active WHERE minute_active BETWEEN ''2016-07-13 12:00:00'' AND ''2016-07-17 12:00:00''';
%sql = 'SELECT widget_id, minute_active, UNIX_TIMESTAMP(minute_active) FROM widget_minute_active WHERE minute_active BETWEEN ''2016-10-06 12:00:00'' AND ''2016-10-09 12:00:00''';

conn = database('widget_activity','ross','woof','Vendor','MySQL','Server','localhost');
curs = exec(conn, sql);
curs = fetch(curs);
results = curs.Data;

widgetIds = uint64(cell2mat(results(:,1)));
widgetMinutes = datenum(cell2mat(results(:,2)));


%% Create a time vector for the x-axis.

% The time vector t will contain the datenum value for each minute over the
% complete time span of the data and the plots.

% Get the start of the time span.
[yr, mon, day, hr, mn, sec] = datevec(cell2mat(plotConfig(2)));

% The time span will be in full-day increments in order to facilitate
% full-day plots.
spanMinutes = ceil(datenum(cell2mat(plotConfig(3))) - datenum(cell2mat(plotConfig(2)))) * 24 * 60;

t = datenum(yr, mon, day, hr, ...
            mn : mn + spanMinutes, ...
            0)';


%% Create a matrix representing when each widget was active.

y = zeros(length(t), 15);

for widgetId = 1 : 15
    % Find the indices for this widget's active minute elements.
    activeMinuteIdxs = find(widgetIds == widgetId);
    % Extract the minutes when this widget was active.
    activeMinutes = widgetMinutes(activeMinuteIdxs);
    % Find the corresponding indices in the x-axis time vector.
    [tFounds, tIdxs] = ismember(activeMinutes, t);
    if ~all(tFounds)
        fprintf('Widget %d has %d unidentified time values.\n', ...
                widgetId, length(find(tFounds == false)));
    end
    % For each minute that the widget was active, set the y value for that
    % widget to the widget's id.  That will allow us to create a plot where
    % stacked, horizontal line segments represent when the widgets were active.
    yIdxs = sub2ind(size(y), tIdxs, repmat(widgetId, length(tIdxs), 1));
    y(yIdxs) = widgetId;
end

% The remaining zeros in the matrix represent periods when a widget was not
% active.  Set each zero value to NaN so that it isn't plotted.  This will
% eliminate the vertical lines that would otherwise appear on the plot.
y(y == 0) = NaN;


%% Create and save the full-event plot.

%figure(fignum);

set(gcf, 'units', 'points', 'position', [10, 10, 1280, 480]);

% adjust the x-axis values by the event's timezone offset
tAdj = t + cell2mat(plotConfig(4)) / 24;

p = plot(tAdj, y);

set(p, 'LineWidth', 1.25);

set(gca, 'XLim', [tAdj(1) tAdj(end)]);
set(gca, 'XTick', tAdj(1) : 24 / 24 : tAdj(end));

datetick('x', 'ddd mm/dd HH:MM', 'keeplimits', 'keepticks');

set(gca, 'YLim', [0 10]);
set(gca, 'YTick', 0 : 10);
set(gca, 'YTickLabel', widgetNames);

xlabel('Time (24-hour notation:  00:00 = midnight, 12:00 = noon)', ...
       'Color', 'black', 'FontSize', 16);
ylabel('Widget', 'Color', 'black', 'FontSize', 16);
title(['Widget Minute-By-Minute Activity at ', cell2mat(plotConfig(1))], ...
       'FontSize', 18)

grid(gca, 'on');

exportOptions.Format = 'epsc';
hgexport(gcf, ['widget activity - ', cell2mat(plotConfig(1))], exportOptions);

close;


%% Create and save the nightly plots.

dayStartIdx = 1;
dayNum = 1;
while dayStartIdx < length(tAdj)
    
    fprintf('dayStartIdx=%d', dayStartIdx);
    dayEndIdx = min(dayStartIdx + 1439, length(tAdj));

    tDay = tAdj(dayStartIdx : dayEndIdx);
    yDay = y(dayStartIdx : dayEndIdx, :);
    
    %figure(fignum);

    set(gcf, 'units', 'points', 'position', [10, 10, 1280, 480]);

    p = plot(tDay, yDay);

    set(p, 'LineWidth', 1.25);

    set(gca, 'XLim', [tDay(1) tDay(end)]);
    set(gca, 'XTick', tDay(1) : 3 / 24 : tDay(end));

    datetick('x', 'ddd mm/dd HH:MM', 'keeplimits', 'keepticks');

    set(gca, 'YLim', [0 10]);
    set(gca, 'YTick', 0 : 10);
    set(gca, 'YTickLabel', widgetNames);

    xlabel('Time (24-hour notation:  00:00 = midnight, 12:00 = noon)', ...
           'Color', 'black', 'FontSize', 16);
    ylabel('Widget', 'Color', 'black', 'FontSize', 16);
    title( ...
        sprintf('Widget Minute-By-Minute Activity at %s - Night %d', ...
                cell2mat(plotConfig(1)), dayNum), ...
        'FontSize', 18)

    grid(gca, 'on');

    exportOptions.Format = 'epsc';
    hgexport(gcf, ...
             sprintf('widget activity - %s - Night %d', ...
                     cell2mat(plotConfig(1)), dayNum), ...
             exportOptions);

    close;

    dayStartIdx = dayStartIdx + 1440;
    dayNum = dayNum + 1;
end
