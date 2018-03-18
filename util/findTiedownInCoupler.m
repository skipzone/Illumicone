function [overlapCount, overlappedCouplers] = findTiedownInCoupler( ...
    ringCircumference, ...
    couplerCenterPositions, ...
    couplerLength, ...
    numTiedowns, ...
    tiedownRadius, ...
    offset)
% findTiedownInCoupler
% Identifies position overlaps between couplers and tiedown points.

overlappedCouplers = zeros(numTiedowns, 2);
overlapCount = 0;

stringTiedownPositions = [0:numTiedowns-1] .* (ringCircumference / numTiedowns) + offset;

for tdIdx = 1 : numTiedowns

    % Find the closest coupler to this tiedown.
    couplerDistances = couplerCenterPositions - stringTiedownPositions(tdIdx);
    closestCouplerIdx = find(abs(couplerDistances) == min(abs(couplerDistances)));
    %display(sprintf('coupler %d is closest to tie-down %d', closestCouplerIdx, tdIdx));

    tiedownDistanceFromCouplerCenter = couplerDistances(closestCouplerIdx);
    if (abs(tiedownDistanceFromCouplerCenter) < couplerLength / 2 + tiedownRadius)
        overlapCount = overlapCount + 1;
        overlappedCouplers(overlapCount, 1) = closestCouplerIdx;
        overlappedCouplers(overlapCount, 2) = tiedownDistanceFromCouplerCenter;
        %display(sprintf('tie-down %d is in coupler %d at %g inches from center', ...
        %    tdIdx, closestCouplerIdx, tiedownDistanceFromCouplerCenter));
    end
end
