function [overlapCount, overlappedCouplers] = ...
    findTiedownInCoupler(ringCircumference, couplerCenterPositions, couplerLength, numStrings, tiedownRadius, offset)
% findTiedownInCoupler
% Identifies position overlaps between couplers and tiedown points.
%
% Ross Butler, January 2018.

overlappedCouplers = zeros(numStrings, 2);
overlapCount = 0;

stringTiedownPositions = [0:numStrings-1] .* (ringCircumference / numStrings) + offset;

for tdIdx = 1 : numStrings

    % Find the closest coupler to this tiedown.
    couplerDistances = couplerCenterPositions - stringTiedownPositions(tdIdx);
    closestCouplerIdx = find(abs(couplerDistances) == min(abs(couplerDistances)));
%        display(sprintf('coupler %d is closest to tie-down %d', closestCouplerIdx, tdIdx));

    tiedownDistanceFromCouplerCenter = couplerDistances(closestCouplerIdx);
    if (abs(tiedownDistanceFromCouplerCenter) < couplerLength / 2 + tiedownRadius)
        overlapCount = overlapCount + 1;
        overlappedCouplers(overlapCount, 1) = closestCouplerIdx;
        overlappedCouplers(overlapCount, 2) = tiedownDistanceFromCouplerCenter;
        display(sprintf('tie-down %d is in coupler %d at %g inches from center', ...
            tdIdx, closestCouplerIdx, tiedownDistanceFromCouplerCenter));
    end
end
