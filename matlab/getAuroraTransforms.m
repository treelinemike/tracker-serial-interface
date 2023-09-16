function all_tfs = getAuroraTransforms(fser,timeout,port_id_list)

pkt = [];
while(isempty(pkt))
    requestAuroraPacket(fser,port_id_list);
    pkt = getAuroraPacket(fser,timeout);
end

assert(mod(length(pkt)-1,9) == 0,'Incorrect packet size!');
num_tforms = (length(pkt)-1)/9;
all_tfs = [];
for tf_idx = 1:num_tforms
    id_uint32 = uint32(pkt(2+9*(tf_idx-1)));
    id_str = deblank(native2unicode(typecast(id_uint32, 'uint8'),'US-ASCII'));
    all_tfs(tf_idx).sn = id_str;
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
