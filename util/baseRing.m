% This MATLAB script calculates and plots the locations of the couplers and
% tie-down points for the new Illumicone base.      Ross, 21 Jan. 2018

clear;
close all;


%% Set the dimensions of the ring and its components.

% All measurements and coordinates are in inches

ringRadius = 19 * 12 + 4;       % the actual size of the physical ring:  19'4"

ringCircumference = 2 * pi * ringRadius;

% This is the best configuration, producing no tie-down/coupler overlaps.
% Unfortunately, we can't make the segments all of equal length because the
% usable portion of each pipe is only 21' - 2' = 19' long, and 19' / 3 = 6' 4".
% numRingSegments = 18;
% segmentLength = 6 * 12 + 9;     % Make all the segments 6'9" long.
% ringSegmentLengths = ones(1, numRingSegments) * segmentLength;

% This appears to be the next-best configuration, producing only 2 overlaps
% for 36 strings and none for 48 strings.  It also makes all segments
% except for the last the same length.
numRingSegments = 19;
segmentLength = 6 * 12 + 4;     % Make all the segments except the last 6'4" long.
ringSegmentLengths = ones(1, numRingSegments) * segmentLength;

% numRingSegments = 20;
% segmentLength = 6 * 12;
% ringSegmentLengths = ones(1, numRingSegments) * segmentLength;

% numRingSegments = 19;
% s1 = 77;
% s2 = 77;
% ringSegmentLengths = [s1 s1 s2 s1 s1 s2 s1 s1 s2 s1 s1 s2 s1 s1 s2 s1 s1 s2 0];

% The last segment's length is whatever is needed to complete the circle.
ringSegmentLengths(numRingSegments) = ...
    ringCircumference - sum(ringSegmentLengths(1:numRingSegments - 1));

couplerLength = 6;

tiedownRadius = 0.5;


%% Set the appearances of the plots.

center = [0 0];

ringLineWidth = 1.25;
ringColor = 'black';
ringN = 2000;
ringLabelRadius = ringRadius * 1.01;

couplerRadius = ringRadius;         % plot couplers on top of the ring
couplerLineWidth = 6;
couplerColor = 'red';
couplerN = 100;

string36Radius = ringRadius * 0.98;
string36Color = 'blue';
string36Marker = 'hexagram';

string48Radius = ringRadius * 0.94;
string48Color = 'green';
string48Marker = 'hexagram';


%% Calculate the coupler positions.

% Calculate the center positions of the couplers on the ring's
% circumference.  The center position is where two segments come together.
couplerCenterPositions = zeros(1, numRingSegments);
for i = 2 : numRingSegments
    couplerCenterPositions(i) = ...
        couplerCenterPositions(i - 1) + ringSegmentLengths(i - 1);
end

% Calculate the starting and ending span of each coupler.
couplerSpanStartPositions = couplerCenterPositions - couplerLength / 2;
couplerSpanEndPositions = couplerCenterPositions + couplerLength / 2;
% Make the first coupler's starting position non-negative. 
couplerSpanStartPositions(1) = couplerSpanStartPositions(1) + ringCircumference;


%% Calculate the tie-down positions, optimizing for the least overlaps with couplers.

string36Offset = findBestOffset( ...
    ringCircumference, ...
    couplerCenterPositions, ...
    couplerLength, ...
    36, ...
    tiedownRadius);
string36TiedownPositions = [0:35] .* (ringCircumference / 36) + string36Offset;

[overlapCount48 overlappedCouplers48] = ...
    findTiedownInCoupler( ...
        ringCircumference, ...
        couplerCenterPositions, ...
        couplerLength, ...
        48, ...
        tiedownRadius, ...
        string36Offset);
display(sprintf( ...
    'The best 36-string offset produces %d 48-string overlaps:', ...
    overlapCount48));
for j = 1 : overlapCount48
    display(sprintf( ...
        '    coupler %d at %.4g" from center', ...
        overlappedCouplers48(j, 1), ...
        overlappedCouplers48(j, 2)));
end

% string48Offset = findBestOffset( ...
%     ringCircumference, ...
%     couplerCenterPositions, ...
%     couplerLength, ...
%     48, ...
%     tiedownRadius);
string48Offset = string36Offset;
string48TiedownPositions = [0:48] .* (ringCircumference / 48) + string48Offset;


%% Plot the entire ring.

[x, y] = arc(center, ringRadius, [0 2*pi], ringN);
h = plot(x, y);
set(h, 'Color', ringColor, 'LineWidth', ringLineWidth);
axis([-1.25*ringRadius+center(1) 1.25*ringRadius+center(1) -1.25*ringRadius+center(2) 1.25*ringRadius+center(2)]);
axis equal;
hold on;

segmentStartAngle = 0;
for i = 1:numRingSegments
    segmentAngle = ringSegmentLengths(i) / ringCircumference * 2 * pi;
    labelAngle = segmentStartAngle + segmentAngle / 2;
    [x, y] = arc(center, ringLabelRadius, [labelAngle labelAngle], 1);
    x = x + center(1);
    y = y + center(2);
    if x >= 0
        halign = 'left';
    else
        halign = 'right';
    end
    if y >= 0
        valign = 'bottom';
    else
        valign = 'top';
    end
    text(x, y, sprintf('%d:  %.4g"', i, ringSegmentLengths(i)), ...
        'VerticalAlignment', valign, 'HorizontalAlignment', halign, ...
        'FontSize', 14, 'Color', ringColor);
    segmentStartAngle = segmentStartAngle + segmentAngle;
end


%% Plot the couplers.

couplerSpanStartAngles = couplerSpanStartPositions / ringCircumference * 2 * pi;
couplerSpanEndAngles = couplerSpanEndPositions / ringCircumference * 2 * pi;

for i = 1:numRingSegments
    [x, y] = arc( ...
        center, ...
        couplerRadius, ...
        [couplerSpanStartAngles(i) couplerSpanEndAngles(i)], ...
        couplerN);
    h = plot(x, y);
    set(h, 'Color', couplerColor, 'LineWidth', couplerLineWidth);
    labelx = x(1);
    labely = y(1);
    if labelx >= 0
        halign = 'left';
    else
        halign = 'right';
    end
    if labely >= 0
        valign = 'bottom';
    else
        valign = 'top';
    end
    text(labelx, labely, cellstr(num2str(i)), ...
        'VerticalAlignment', valign, 'HorizontalAlignment', halign, ...
        'Color', 'red');
end


%% Plot the 36-string configuration tie-down points.

string36TiedownAngles = ...
    string36TiedownPositions / ringCircumference * 2 * pi;
for i = 1 : 36
    [x, y] = arc( ...
        center, ...
        string36Radius, ...
        [string36TiedownAngles(i) string36TiedownAngles(i)], ...
        1);
    h = plot(x, y, 'Marker', string36Marker, 'Color', string36Color);
    if x >= 0
        halign = 'right';
    else
        halign = 'left';
    end
    if y >= 0
        valign = 'top';
    else
        valign = 'bottom';
    end
    text(x, y, cellstr(num2str(i)), ...
        'VerticalAlignment', valign, 'HorizontalAlignment', halign, ...
        'Color', string36Color);
end


%% Plot the 48-string configuration tie-down points.

string48TiedownAngles = ...
    string48TiedownPositions / ringCircumference * 2 * pi;
for i = 1 : 48
    [x, y] = arc( ...
        center, ...
        string48Radius, ...
        [string48TiedownAngles(i) string48TiedownAngles(i)], ...
        1);
    h = plot(x, y, 'Marker', string48Marker, 'Color', string48Color);
    if x >= 0
        halign = 'right';
    else
        halign = 'left';
    end
    if y >= 0
        valign = 'top';
    else
        valign = 'bottom';
    end
    text(x, y, cellstr(num2str(i)), ...
        'VerticalAlignment', valign, 'HorizontalAlignment', halign, ...
        'Color', string48Color);
end

