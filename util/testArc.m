
center = [0 0];
r = 5;
%span = [pi/4 pi/3];
%span = [pi/4 pi/3];
span = [pi/3 2*pi/3];
N = 100

[x, y] = arc(center, r, span, N);

style = 'k-';
h = plot(x, y, style);
axis([-1.25*r+center(1) 1.25*r+center(1) -1.25*r+center(2) 1.25*r+center(2)]);
axis equal;
