function [x, y] = arc(center, radius, span, N)
% ARC
% Calculates N points on an angle arc with specified position and circumference.
% Example:
%                 [x, y] = arc([3 2], 5, [0 pi/2]);
% Returns N points on the arc with center (3,2) and radius (5) that represents
% the angle specified from 0 to pi/2.
% Draws heavily from Husam Aldahiyat's ang function on the File Exchange.
%
% Ross Butler, January 2018.

theta = linspace(span(1), span(2), N);
rho = ones(1, N) * radius;

[x,y] = pol2cart(theta,rho);

x = x + center(1);
y = y + center(2);

end