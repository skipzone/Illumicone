clear;
format long;


%% Get the widget minute-by-minute activity data from the database.

conn = database('widget_activity','ross','woof','Vendor','MySQL','Server','localhost');
%sql = 'SELECT widget_id, minute_active, UNIX_TIMESTAMP(minute_active) FROM widget_minute_active';
%sql = 'SELECT widget_id, minute_active, UNIX_TIMESTAMP(minute_active) FROM widget_minute_active WHERE minute_active BETWEEN ''2016-08-26 18:00:00'' AND ''2016-09-04 18:00:00''';
%sql = 'SELECT widget_id, minute_active, UNIX_TIMESTAMP(minute_active) FROM widget_minute_active WHERE minute_active BETWEEN ''2016-07-13 12:00:00'' AND ''2016-07-17 12:00:00''';
sql = 'SELECT widget_id, minute_active, UNIX_TIMESTAMP(minute_active) FROM widget_minute_active WHERE minute_active BETWEEN ''2016-10-06 12:00:00'' AND ''2016-10-09 12:00:00''';
curs = exec(conn, sql);
curs = fetch(curs);
results = curs.Data;

widgetIds = uint64(cell2mat(results(:,1)));
widgetMinutes = datenum(cell2mat(results(:,2)));
%widgetMinutesUnixTime = int32(cell2mat(results(:,3)));


%% Create a time vector for the x-axis.

% The time vector t will contain the datenum value for each minute over the
% complete time span of the data.

%startDateVec = datevec(min(widgetMinutes));
[yr, mon, day, hr, mn, sec] = datevec(min(widgetMinutes));

% +1 is fudge factor needed because of accumulated fractional day error.
t = datenum(yr, mon, day, hr, ...
            mn : mn + (max(widgetMinutes) - min(widgetMinutes)) * 24 * 60 + 1, ...
            0);

% Subtract 6/24 because t is MDT, not UTC.
%tUnixTime = int32(86400 * (t - (datenum('01-Jan-1970') - 6 / 24)));


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
        fprintf('Widget %d has %d unidentified time values.\n', widgetId, length(find(tFounds == false)));
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


%% Create the plots.

%figure(fignum);

p = plot(t, y);

set(gca, 'XLim', [t(1) t(end)]);
set(gca, 'XTick', [t(1) : 12/24 : t(end)]);

datetick('x', 'ddd mm/dd HH:MM', 'keeplimits', 'keepticks');

set(gca, 'YLim', [0 10]);
set(gca, 'YTick', [0 : 10]);
set(gca, 'YTickLabel', {' '; 'Eye'; 'Shirleys Web'; 'Bells'; 'Steps'; ' '; 'TriObelisk'; ' '; 'Plunger'; 'FourPlay'; ' '});

xlabel('Date and Time', 'Color', 'black', 'FontSize', 16);
ylabel('Widget', 'Color', 'black', 'FontSize', 16);
title('Widget Minute-By-Minute Activity')

set(p, 'LineWidth', 1.25);

grid(gca, 'on');


%% 

% Validate conversion of datenum values to Unix time values.
% Subtract 6/24 because widgetMinutes is MDT, not UTC.
%unixTimeFromDatenum = int32(86400 * (widgetMinutes - (datenum('01-Jan-1970') - 6 / 24)));
%unixTimeDiff = widgetMinutesUnixTime - unixTimeFromDatenum;

% Two ways to convert unix time to datenum:
% datenum([1970 1 1 0 0 unix_time])
% unix_time/86400 + datenum(1970,1,1)

