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


%% Set the appearances of the plots.

center = [0 0];

ringLineWidth = 1.25;
ringColor = 'black';
ringN = 2000;

couplerRadius = ringRadius;     % plot couplers on top of the ring
couplerLineWidth = 6;
couplerColor = 'red';
couplerN = 100;


%% Calculate the pipe segment lengths and the coupler positions.

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


%% Plot the entire ring in black.
[x, y] = arc(center, ringRadius, [0 2*pi], ringN);
h = plot(x, y);
set(h, 'Color', ringColor, 'LineWidth', ringLineWidth);
axis([-1.25*ringRadius+center(1) 1.25*ringRadius+center(1) -1.25*ringRadius+center(2) 1.25*ringRadius+center(2)]);
axis equal;
hold on;


%% Plot the couplers.

couplerCenterPositionAngles = couplerCenterPositions / ringCircumference * 2 * pi;
couplerHalfLengthAngle = couplerLength / 2 / ringCircumference * 2 * pi;
couplerSpanStartAngles = couplerCenterPositionAngles - couplerHalfLengthAngle;
couplerSpanEndAngles = couplerCenterPositionAngles + couplerHalfLengthAngle;
for i = 1 : numRingSegments
    [x, y] = arc( ...
        center, ...
        couplerRadius, ...
        [couplerSpanStartAngles(i) couplerSpanEndAngles(i)], ...
        couplerN);
    h = plot(x, y);
    set(h, 'Color', couplerColor, 'LineWidth', couplerLineWidth);
end

