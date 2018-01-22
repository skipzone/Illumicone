clear;

%% Set the dimensions of the ring and its components.

% All measurements and coordinates are in inches

ringRadius = 19 * 12 + 4;       % the actual size of the physical ring

% We can't make the segments all of equal length because the usable portion
% of each pipe is only 21' - 2' = 19' long, and 19' / 3 = 6' 4".
%numRingSegments = 18;
%segmentLength = 6 * 12 + 9;     % Make all the segments 6'9" long.

numRingSegments = 19;
segmentLength = 6 * 12 + 4;     % Make all the segments except the last 6'4" long.

couplerLength = 6;

tiedownRadius = 0.25;


%% Set the appearances of the plots.

center = [0 0];

ringLineWidth = 1.25;
ringColor = 'black';
ringN = 2000;
ringLabelRadius = ringRadius * 1.01;

couplerRadius = ringRadius;     % plot couplers on top of the ring
couplerLineWidth = 6;
couplerColor = 'red';
couplerN = 100;

string36Radius = ringRadius * 0.98;
string36Symbol = 'bp';              % plot these tie-down points as blue stars

string48Radius = ringRadius * 0.94;
string48Symbol = 'gp';              % plot these tie-down points as green stars


%% Calculate the pipe segment lengths and coupler positions.

ringCircumference = 2 * pi * ringRadius;

% Set all segments to the specified length.
ringSegmentLengths = ones(1, numRingSegments) * segmentLength;
% The last segment's length is whatever is needed to complete the circle.
ringSegmentLengths(numRingSegments) = ...
    ringCircumference - sum(ringSegmentLengths(1:numRingSegments - 1));

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

offsetStep = tiedownRadius;

numTiedowns = 36;

offsets = ...
    couplerLength / 2 + tiedownRadius ...
    : offsetStep ...
    : ringCircumference / numTiedowns - (couplerLength / 2 + tiedownRadius);

overlapCount = zeros(size(offsets));
overlappedCouplers = zeros(size(offsets, 2), numTiedowns, 2);

for offsetIdx = 1 : size(offsets, 2)
    display(sprintf('----- offset %d = %g inches -----', offsetIdx, offsets(offsetIdx)));
    [overlapCount(offsetIdx) overlappedCouplers(offsetIdx, :, :)] = ...
        findTiedownInCoupler( ...
            ringCircumference, ...
            couplerCenterPositions, ...
            couplerLength, ...
            numTiedowns, ...
            tiedownRadius, ...
            offsets(offsetIdx));
end

% Identify the offsets that produce the fewest tie-downs located in a
% coupler.
fewestConflictIdxs = find(overlapCount == min(overlapCount));

% The best offset is the one that, overall, places the tie-downs farthest from
% the centers of the couplers, thus required the smallest overall adjustment of
% the tie-downs to move them out of the couplers.  We use the log of the
% distance to favor offsets having tie-downs a moderate distance from
% coupler centers over those having an offset very far from a coupler
% center but another very near.
weightedDistances = ...
    sum(log(abs(overlappedCouplers(fewestConflictIdxs, :, 2)) + 1), 2);
bestOffsetIdx = ...
    fewestConflictIdxs( ...
        find(weightedDistances == max(weightedDistances), 1));

display(sprintf( ...
    'best %d-string offset is %d, measuring %g inches and producing %d overlaps:', ...
    numTiedowns, bestOffsetIdx, offsets(bestOffsetIdx), overlapCount(bestOffsetIdx)));
for j = 1 : overlapCount(bestOffsetIdx)
    display(sprintf( ...
        '    coupler %d at %.4g" from center', ...
        overlappedCouplers(bestOffsetIdx, j, 1), ...
        overlappedCouplers(bestOffsetIdx, j, 2)));
end

string36Offset = offsets(bestOffsetIdx);
string36TiedownPositions = [0:35] .* (ringCircumference / 36) + string36Offset;



string48Offset = string36Offset;

string48TiedownPositions = [0:48] .* (ringCircumference / 48) + string48Offset;


%% Plot the entire ring in black.
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
        'FontSize', 14);
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
    h = plot(x, y, string36Symbol);
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
        'Color', 'blue');
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
    h = plot(x, y, string48Symbol);
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
        'Color', 'green');
end

