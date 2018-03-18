function [x, y] = arc(center, radius, span, N)
% arc
% Calculates N points of the arc that spans the angle range [span(1), span(2)]
% on a circle with the specified center position and radius.
% Example:
%                 [x, y] = arc([3 2], 5, [0 pi/2]);
% Returns N points on the arc with center (3,2) and radius (5) that represents
% the angle specified from 0 to pi/2.
% Draws heavily from Husam Aldahiyat's ang function on the File Exchange.
%
% Ross Butler, January 2018.

% Normalize the start of the span to be numerically less than the end of
% the span.
while span(1) > span(2)
    span(1) = span(1) - 2 * pi;
end

theta = linspace(span(1), span(2), N);
rho = ones(1, N) * radius;

[x,y] = pol2cart(theta, rho);

x = x + center(1);
y = y + center(2);

end