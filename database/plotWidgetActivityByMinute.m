%{

--------------------------------------------------------------------------------
This MATLAB script creates widget activity plots.

Ross Butler  November 2016
--------------------------------------------------------------------------------

This file is part of Illumicone.

Illumicone is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Illumicone is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Illumicone.  If not, see <http://www.gnu.org/licenses/>.

%}


%% Initialization and configuration

clear;

format long;

% Most of these are non-standard (for MATLAB) colors normalized to MATLAB values.
% (source:  http://kb.iu.edu/data/aetf.html)
rgbOrange               = [255 140   0] ./ 255;
rgbBlueViolet           = [138  43 226] ./ 255;
rgbDarkMagenta          = [128   0 128] ./ 255;
rgbDarkRed              = [128   0   0] ./ 255;
rgbDarkGreen            = [  0 128   0] ./ 255;
rgbDarkBlue             = [  0   0 128] ./ 255;
rgbDarkGoldenrod        = [184 134  11] ./ 255;
rgbDeepPink             = [255  20 147] ./ 255;
rgbMediumSpringGreen    = [  0 250 154] ./ 255;
rgbOliveDrab            = [107 142  35] ./ 255;
rgbSeaGreen             = [ 46 139  87] ./ 255;
rgbBrown                = [165  42  42] ./ 255;
rgbChocolate            = [210 105  30] ./ 255;
rgbChartreuse           = [127 255   0] ./ 255;
rgbCadetBlue            = [ 95 158 160] ./ 255;
rgbAquamarine           = [127 255 212] ./ 255;
rgbCornflowerBlue       = [100 149 237] ./ 255;
rgbTeal                 = [  0 128 128] ./ 255;
rgbRed                  = [255   0   0] ./ 255;
rgbBlue                 = [  0   0 255] ./ 255;
rgbGray                 = [128 128 128] ./ 255;
rgbDarkGray             = [ 64  64  64] ./ 255;           

plotConfigs = [ ...
    {'Element 11 2016'      % event description for plot title
     '2016-07-13 12:00'     % start date and time (time should be noon)
     '2016-07-17 11:59:59'  % end date and time (time should be 11:59:59)
     0                      % venue timezone hour offset from data timezone
     }, ...
     {'Burning Man 2016'
      '2016-08-26 13:00'
      '2016-09-05 12:59:59'
      -1
     }, ...
     {'Idaho Decompression 2016'
      '2016-10-06 12:00'
      '2016-10-09 11:59:59'
      0
     }
];

widgetNames = {
               'Eye';           % 1
               'Shirleys Web';  % 2
               'Bells';         % 3
               'Steps';         % 4
               ' ';             % 5 (Rain Stick)
               'TriObelisk';    % 6
               ' ';             % 7 (Box Theramin)
               'Plunger';       % 8
               'FourPlay';      % 9
               ' '              % 10 (unassigned; for blank tick label at top of plot)
};

%%
widgetColors = [
    rgbDarkBlue
    rgbDarkGreen
    rgbDarkMagenta
    rgbDarkRed
    rgbOrange
    rgbBlueViolet
    rgbDarkGoldenrod
    rgbDeepPink
    rgbTeal
    rgbCornflowerBlue
];
          
conn = database('widget_activity','ross','woof','Vendor','MySQL','Server','localhost');

    
%% Do plot generation for each event.

for plotConfig = plotConfigs
           
              
    %% Get the widget minute-by-minute activity data from the database.

    sql = [ ...
        'SELECT widget_id, minute_active' ...
        ' FROM widget_minute_active' ...
        ' WHERE minute_active BETWEEN ''', cell2mat(plotConfig(2)), '''' ...
        '   AND ''', cell2mat(plotConfig(3)), ''''];

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

    y = zeros(length(t), length(widgetNames));

    for widgetId = 1 : length(widgetNames)
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

    set(gcf, 'units', 'points', 'position', [10, 10, 1280, 480]);

    % adjust the x-axis values by the event's timezone offset
    tAdj = t + cell2mat(plotConfig(4)) / 24;

    p = plot(tAdj, y);

    for lineSeriesIdx = 1 : length(p)
        set(p(lineSeriesIdx), 'LineWidth', 1.25, 'Color', widgetColors(lineSeriesIdx, :));
    end

    set(gca, 'XLim', [tAdj(1) tAdj(end)]);
    set(gca, 'XTick', tAdj(1) : 24 / 24 : tAdj(end));

    datetick('x', 'ddd mm/dd HH:MM', 'keeplimits', 'keepticks');

    set(gca, 'YLim', [0 10]);
    set(gca, 'YTick', 1 : 10);
    set(gca, 'YTickLabel', widgetNames);

    xlabel('Time (24-hour notation:  00:00 = midnight, 12:00 = noon)', ...
           'Color', 'black', 'FontSize', 16);
    ylabel('Widget', 'Color', 'black', 'FontSize', 16);
    title(['Widget Activity at ', cell2mat(plotConfig(1))], ...
           'FontSize', 18)

    grid(gca, 'on');

    exportOptions.Format = 'epsc';
    hgexport(gcf, ['widget activity - ', cell2mat(plotConfig(1))], exportOptions);

    close;


    %% Create and save the nightly plots.

    dayStartIdx = 1;
    dayNum = 1;
    while dayStartIdx < length(tAdj)

        dayEndIdx = min(dayStartIdx + 1439, length(tAdj));

        tDay = tAdj(dayStartIdx : dayEndIdx);
        yDay = y(dayStartIdx : dayEndIdx, :);

        set(gcf, 'units', 'points', 'position', [10, 10, 1280, 480]);

        p = plot(tDay, yDay);

        for lineSeriesIdx = 1 : length(p)
            set(p(lineSeriesIdx), 'LineWidth', 1.25, 'Color', widgetColors(lineSeriesIdx, :));
        end

        set(gca, 'XLim', [tDay(1) tDay(end)]);
        set(gca, 'XTick', tDay(1) : 3 / 24 : tDay(end));

        datetick('x', 'ddd mm/dd HH:MM', 'keeplimits', 'keepticks');

        set(gca, 'YLim', [0 10]);
        set(gca, 'YTick', 1 : 10);
        set(gca, 'YTickLabel', widgetNames);

        xlabel('Time (24-hour notation:  00:00 = midnight, 12:00 = noon)', ...
               'Color', 'black', 'FontSize', 16);
        ylabel('Widget', 'Color', 'black', 'FontSize', 16);
        title( ...
            sprintf('Widget Activity at %s - Night %d', ...
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


%% End of plot-generation-for-event loop.

end

