% restart
close all; clear; clc;

PROBE_ID_PENCOIL  = '0A';   % pen probe coil
PROBE_ID_SPINE    = '0B';   % Brook's spine coil

fser = serialport('COM13',115200,'DataBits',8,'FlowControl','none','StopBits',1,'Timeout',0.001);

pkt = [];
while(isempty(pkt))
    requestAuroraPacket(fser,{PROBE_ID_PENCOIL PROBE_ID_SPINE});
    pkt = getAuroraPacket(fser,0.2);
end

clear fser;

assert(mod(length(pkt)-1,9) == 0,'Incorrect packet size!');
num_tforms = (length(pkt)-1)/9;
all_tfs = [];
for tf_idx = 1:num_tforms
    all_tfs(tf_idx).sn = pkt(2+9*(tf_idx-1));
    all_tfs(tf_idx).T_coil_to_aurora = eye(4);
    all_tfs(tf_idx).T_coil_to_aurora(1:3,1:3) = quat2matrix(pkt((3:6)+9*(tf_idx-1)));
    all_tfs(tf_idx).T_coil_to_aurora(1:3,4) = pkt((7:9)+9*(tf_idx-1));
    all_tfs(tf_idx).error = pkt(10+9*(tf_idx-1));
end
if(num_tforms)
    all_tfs.T_coil_to_aurora
else
    warning('No transforms returned!');
end


 