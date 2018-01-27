% This MATLAB script calculates and plots the locations of the couplers and
% tie-down points for the new Illumicone base.      Ross, 21 Jan. 2018

clear;
close all;

of = fopen('baseRing.txt','w');


%% Set the dimensions of the ring and its components.

% All measurements and coordinates are in inches.

ringInsideRadius = 19 * 12 + 4;     % the actual size of the physical ring:  19'4"
pipeOutsideDiameter = 2.375;        % for converting inside to outside measurements
pipeWeightPerInch = 3.66 / 12;      % lbs per inch
couplerWeightPerInch = 5.80 / 12;   % lbs per inch

ringOutsideRadius = ringInsideRadius + pipeOutsideDiameter;
ringInsideCircumference = 2 * pi * ringInsideRadius;
ringOutsideCircumference = 2 * pi * ringOutsideRadius;
ringInsideToOutsideCircumferenceMultiplier = ...
    ringOutsideCircumference / ringInsideCircumference;

% Each 252" (21') pipe has 7"-10" of unusable, straight length at each end due
% to the rolling process.  The net usable length of each pipe is 232".

% This is the best configuration, producing no tie-down/coupler overlaps.
% Unfortunately, we can't make the segments all of equal length because the
% usable portion of each pipe is only 21' - 2' = 19' long, and 19' / 3 = 6' 4".
% numRingSegments = 18;
% segmentLength = 6 * 12 + 9;     % Make all the segments 6'9" long.
% ringSegmentInsideLengths = ones(1, numRingSegments) * segmentLength;

% This appears to be the next-best configuration, producing only 2 overlaps
% for 36 strings and none for 48 strings.  It also makes all segments
% except for the last the same length.
% numRingSegments = 19;
% segmentLength = 6 * 12 + 4;     % Make all the segments except the last 6'4" long.
% ringSegmentInsideLengths = ones(1, numRingSegments) * segmentLength;

% numRingSegments = 20;
% segmentLength = 6 * 12;
% ringSegmentInsideLengths = ones(1, numRingSegments) * segmentLength;

%numRingSegments = 19;
% s1 = 75;
% s2 = 77;
%ringSegmentInsideLengths = [s1 s1 s2 s1 s1 s2 s1 s1 s2 s1 s1 s2 s1 s1 s2 s1 s1 s2 0];
%ringSegmentInsideLengths = [s1 s2 s1 s2 s1 s2 s1 s2 s1 s2 s1 s2 s1 s2 s1 s2 s1 s2 0];

% This is the huckleberry!
numRingSegments = 19;
%                            1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19
ringSegmentInsideLengths = [75 76 73 80 75 77 75 77 75 77 79 73 75 77 75 77 79 79 0];

% The last segment's length is whatever is needed to complete the circle.
ringSegmentInsideLengths(numRingSegments) = ...
    ringInsideCircumference - sum(ringSegmentInsideLengths(1:numRingSegments - 1));

couplerLength = 6;

tiedownRadius = 0.5;

fprintf(of, 'All lengths and locations are in inches.\n\n');
fprintf(of, 'ring inside radius:  %g\n', ringInsideRadius);
fprintf(of, 'pipe outside diameter:  %g\n', pipeOutsideDiameter);
fprintf(of, 'ring inside circumference:  %g\n', ringInsideCircumference);
fprintf(of, 'ring outside circumference:  %g\n', ringOutsideCircumference);
fprintf(of, 'inside-to-outside multiplier:  %.7g\n', ringInsideToOutsideCircumferenceMultiplier);
fprintf(of, 'number of segments:  %g\n', numRingSegments);
fprintf(of, 'segment lengths (inside):\n');
fprintf(of, ' %4d', [1 : numRingSegments]);
fprintf(of, '\n');
fprintf(of, ' %4.1f', ringSegmentInsideLengths);
fprintf(of, '\nsegment lengths (outside):\n');
fprintf(of, ' %5d', [1 : numRingSegments]);
fprintf(of, '\n');
fprintf(of, ' %5.2f', ringSegmentInsideLengths * ringInsideToOutsideCircumferenceMultiplier);
fprintf(of, '\ncoupler length:  %g\n', couplerLength);
fprintf(of, 'total weight:  %g lbs\n', ...
    (ringInsideCircumference + ringOutsideCircumference) / 2 * pipeWeightPerInch ...
    + couplerLength * numRingSegments * couplerWeightPerInch);


%% Set the appearances of the plots.

center = [0 0];

ringLineWidth = 1.25;
ringColor = 'black';
ringN = 2000;
ringLabelRadius = ringInsideRadius * 1.01;

couplerRadius = ringInsideRadius;         % plot couplers on top of the ring
couplerLineWidth = 6;
couplerColor = 'red';
couplerN = 100;

string36Radius = ringInsideRadius * 0.98;
string36Color = 'blue';
string36Marker = 'hexagram';

string48Radius = ringInsideRadius * 0.94;
string48Color = 'green';
string48Marker = 'hexagram';


%% Calculate the coupler positions.

% Calculate the center positions of the couplers on the ring's
% circumference.  The center position is where two segments come together.
couplerCenterPositions = zeros(1, numRingSegments);
for i = 2 : numRingSegments
    couplerCenterPositions(i) = ...
        couplerCenterPositions(i - 1) + ringSegmentInsideLengths(i - 1);
end

% Calculate the starting and ending span of each coupler.
couplerSpanStartPositions = couplerCenterPositions - couplerLength / 2;
couplerSpanEndPositions = couplerCenterPositions + couplerLength / 2;
% Make the first coupler's starting position non-negative. 
couplerSpanStartPositions(1) = couplerSpanStartPositions(1) + ringInsideCircumference;


%% Calculate the tie-down positions, optimizing for the least overlaps with couplers.

string36Offset = findBestOffset( ...
    of, ...
    ringInsideCircumference, ...
    couplerCenterPositions, ...
    couplerLength, ...
    36, ...
    tiedownRadius);
string36TiedownPositions = [0:35] .* (ringInsideCircumference / 36) + string36Offset;

[overlapCount48 overlappedCouplers48] = ...
    findTiedownInCoupler( ...
        ringInsideCircumference, ...
        couplerCenterPositions, ...
        couplerLength, ...
        48, ...
        tiedownRadius, ...
        string36Offset);
fprintf(of, ...
    'The best 36-string offset produces %d 48-string overlaps:\n', ...
    overlapCount48);
for j = 1 : overlapCount48
    fprintf(of, ...
        '    coupler %d at %.4g" from center\n', ...
        overlappedCouplers48(j, 1), ...
        overlappedCouplers48(j, 2));
end

% string48Offset = findBestOffset( ...
%     ringCircumference, ...
%     couplerCenterPositions, ...
%     couplerLength, ...
%     48, ...
%     tiedownRadius);
% We'll use the same offset for the 48-string configuration so that every
% third of the 36 strings will be in the same place as every fourth of the
% 48 strings.
string48Offset = string36Offset;
string48TiedownPositions = [0:48] .* (ringInsideCircumference / 48) + string48Offset;


%% Print the tie-down locations.

fprintf(of, '\n36-String Tie-Down Locations\n');
fprintf(of, '--------------------------------\n');
fprintf(of, '              Inside    Outside\n');
fprintf(of, 'No.  Segment  Distance  Distance\n');
for tdIdx = 1 : 36    
    % Find the closest coupler before this tiedown.
    previousCouplerIdxs = find(couplerCenterPositions < string36TiedownPositions(tdIdx));
    previousCouplerIdx = max(previousCouplerIdxs);

    tiedownInsidePosition = ...
        string36TiedownPositions(tdIdx) - couplerCenterPositions(previousCouplerIdx);
    tiedownOutsidePosition = ...
        tiedownInsidePosition * ringInsideToOutsideCircumferenceMultiplier;
    
    fprintf(of, '%2d   %2d       %4.1f      %4.1f\n', ...
        tdIdx, previousCouplerIdx, tiedownInsidePosition, tiedownOutsidePosition);
end

fprintf(of, '\n48-String Tie-Down Locations\n');
fprintf(of, '--------------------------------\n');
fprintf(of, '              Inside    Outside\n');
fprintf(of, 'No.  Segment  Distance  Distance\n');
for tdIdx = 1 : 48    
    % Find the closest coupler before this tiedown.
    previousCouplerIdxs = find(couplerCenterPositions < string48TiedownPositions(tdIdx));
    previousCouplerIdx = max(previousCouplerIdxs);

    tiedownInsidePosition = ...
        string48TiedownPositions(tdIdx) - couplerCenterPositions(previousCouplerIdx);
    tiedownOutsidePosition = ...
        tiedownInsidePosition * ringInsideToOutsideCircumferenceMultiplier;
    
    fprintf(of, '%2d   %2d       %4.1f      %4.1f\n', ...
        tdIdx, previousCouplerIdx, tiedownInsidePosition, tiedownOutsidePosition);
end


%% Plot the entire ring.

[x, y] = arc(center, ringInsideRadius, [0 2*pi], ringN);
h = plot(x, y);
set(h, 'Color', ringColor, 'LineWidth', ringLineWidth);
axis([-1.25*ringInsideRadius+center(1) 1.25*ringInsideRadius+center(1) -1.25*ringInsideRadius+center(2) 1.25*ringInsideRadius+center(2)]);
axis equal;
hold on;

segmentStartAngle = 0;
for i = 1:numRingSegments
    segmentAngle = ringSegmentInsideLengths(i) / ringInsideCircumference * 2 * pi;
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
    text(x, y, sprintf('%d:  %.4g" / %.4g"', ...
            i, ringSegmentInsideLengths(i), ringSegmentInsideLengths(i) * ringInsideToOutsideCircumferenceMultiplier), ...
        'VerticalAlignment', valign, 'HorizontalAlignment', halign, ...
        'FontSize', 14, 'Color', ringColor);
    segmentStartAngle = segmentStartAngle + segmentAngle;
end


%% Plot the couplers.

couplerSpanStartAngles = couplerSpanStartPositions / ringInsideCircumference * 2 * pi;
couplerSpanEndAngles = couplerSpanEndPositions / ringInsideCircumference * 2 * pi;

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
    string36TiedownPositions / ringInsideCircumference * 2 * pi;
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
    string48TiedownPositions / ringInsideCircumference * 2 * pi;
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


%% Done.

fclose(of);
