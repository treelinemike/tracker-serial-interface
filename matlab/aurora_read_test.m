% restart
close all; clear; clc;

aurora_tab = readtable('example.csv','Delimiter',',','FileType','text');

figure;
hold on; grid on; axis equal;
ph(1) = plot3(aurora_tab.Tx,aurora_tab.Ty,aurora_tab.Tz,'.','MarkerSize',10,'Color',[0 0 0.8]);
legend(ph,{'Tool 1'});
xlabel('\bfx');
ylabel('\bfy');
zlabel('\bfz');