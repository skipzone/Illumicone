% bingreedy.m - greedy one-dimensional bin-packing algorithms demonstration
% first-fit, best-fit, first-fit-decreasing, best-fit decreasing implemented
% data structure:
% u(1:n): size of n objects,
% C: capacity of each bin
% w(n, n): assignment matrix. w(i,j)= 1 if u(i) is assigned to b(j)
% r(1:n): residue capacity, initially, r = C*ones(1,n);
%
% (C) 2004 by Yu Hen Hu
% created: 9/3/2004 for ECE 556

clear all

% load test data
%u = [1 4 2 1 2 3 5];
%C = 6;
u = [75.0 76.0 73.0 80.0 75.0 77.0 75.0 77.0 75.0 77.0 79.0 73.0 75.0 77.0 75.0 77.0 79.0 79.0 83.7];   % final pipe lengths needed
C = 232;    % usable length of each uncut pipe
n = length(u); 
r = C*ones(1,n);
w = zeros(n); 

% this part of statements will be used by both FFD and BFD algorithms
[ud1,idec]=sort(-u); % sort obj. in decreasing size order
ud=-ud1;
[tmp,irec]=sort(idec); % irec is the recovery ordering
% note that ud=u(idec); and u = ud(irec); 

disp('one dimensional bin-packing heuristic algorithms demonstration');
disp('0 - these methods in sequence (default choice);');
disp('1 - first fit method;');
disp('2 - best fit method;');
disp('3 - first fit decreasing method;');
disp('4 - best fit decreasing method;');
chos=input('Enter your choice: ');
if isempty(chos), chos=0; end

% **********************************************************
if chos==0 | chos==1,  % first fit method
% first fit method (FF)
% for each object i, find the first bin that has sufficient capacity to 
% pack it in. Assign object i to that bin and update the bin's 
% remaining capacity

wff=w; rff=r; % initialize for first fit method
for i=1:n,
   % disp(['u(' int2str(i) ') = ' int2str(u(i))]);
   % disp('r = '); disp(r);
   idx=min(find([u(i)*ones(1,n)<=rff]));
   % disp(['u(' int2str(i) ') is assigned to b(' int2str(idx) ');']);
   wff(i,idx)=1; rff(idx)=rff(idx)-u(i);  % update the state
   % disp('Press any key to continue ...'); pause
end
disp('% *******************************');
disp('First fit (FF) method: ')
disp('the assignment are:')
disp(wff)
occpff=u*wff; 
nbinff=sum([occpff > 0]);
disp(['Total # of bins used = ' int2str(nbinff)]); 
disp('the occupancy of each bin after packing are:');
disp(occpff(1:nbinff))
% disp('Press any key to continue to Best fit method ...'); pause
end
% **********************************************************
if chos==0 | chos==2,  % best fit method
% best fit method (BF)
% for each object i, find the bin whose remaining capacity best
% match the object size, and assign it to that bin
% 
wbf=w; rbf=r; % initialize for best fit method
for i=1:n, 
   % disp(['u(' int2str(i) ') = ' int2str(u(i))]);
   % disp('r = '); disp(r);
   idx1=find([u(i)*ones(1,n) <= rbf]); % indices of bins still have spaces left
   [tmp,idx2]=min(rbf(idx1)-u(i));    % find the index within idx1 that best fits u(i)
   wbf(i,idx1(idx2))=1; rbf(idx1(idx2))=tmp; % update the state 
   % disp('Press any key to continue ...'); pause
end
disp('% *******************************');
disp('Best fit (BF) method: ')
disp('the assignment are:')
disp(wbf)
occpbf=u*wbf; 
nbinbf=sum([occpbf > 0]);
disp(['Total # of bins used = ' int2str(nbinbf)]); 
disp('the occupancy of each bin after packing are:');
disp(occpbf(1:nbinbf))
% disp('Press any key to continue to Best fit method ...'); pause
end

% **********************************************************
if chos==0 | chos==3,   % frist fit decreasing method
   
% first fit decreasing method (FFD)
% sort u in decreasing order and then use FFD method

wffd=w; rffd=r; % initialize for first fit method
for i=1:n,
   % disp(['ud(' int2str(i) ') = ' int2str(ud(i))]);
   % disp('r = '); disp(rffd);
   idx=min(find([ud(i)*ones(1,n)<=rffd]));
   % disp(['u(' int2str(i) ') is assigned to b(' int2str(idx) ');']);
   wffd(i,idx)=1; rffd(idx)=rffd(idx)-ud(i);  % update the state
   % disp('Press any key to continue ...'); pause
end
wffd=wffd(irec,:); % sort it back to the original assignment order
disp('% *******************************');
disp('First fit decreasing (FFD) method: ')
disp('the assignment are:')
disp(wffd)
occpffd=u*wffd; 
nbinffd=sum([occpffd > 0]);
disp(['Total # of bins used = ' int2str(nbinffd)]); 
disp('the occupancy of each bin after packing are:');
disp(occpffd(1:nbinffd))
% disp('Press any key to continue to Best fit method ...'); pause
end

% **********************************************************
if chos==0 | chos==4,  % best fit decreasing method
% best fit decreasing method (BFD)
% sort object in decreasing order and then use best fit
% 
wbfd=w; rbfd=r; % initialize for best fit method
for i=1:n, 
   % disp(['u(' int2str(i) ') = ' int2str(ud(i))]);
   % disp('r = '); disp(r);
   idx1=find([ud(i)*ones(1,n) <= rbfd]); % indices of bins still have spaces left
   [tmp,idx2]=min(rbfd(idx1)-ud(i));    % find the index within idx1 that best fits ud(i)
   wbfd(i,idx1(idx2))=1; rbfd(idx1(idx2))=tmp; % update the state 
   % disp('Press any key to continue ...'); pause
end
wbfd=wbfd(irec,:); % sort it back to the original assignment order
disp('% *******************************');
disp('Best fit decreasing (BFD) method: ')
disp('the assignment are:')
disp(wbfd)
occpbfd=u*wbfd; 
nbinbfd=sum([occpbfd > 0]);
disp(['Total # of bins used = ' int2str(nbinbfd)]); 
disp('the occupancy of each bin after packing are:');
disp(occpbfd(1:nbinbfd))
% disp('Press any key to continue to Best fit method ...'); pause

end % 

