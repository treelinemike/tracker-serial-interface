% simple CRC8 implementation
% probably pretty slow...
function CRC = crcAddBytes(CRC,byte_array)
CRC = uint8(CRC);
for byte_idx = 1:length(byte_array)
    for bit_num = 8:-1:1
        thisBit = ~(0 == ( bitand(uint8(byte_array(byte_idx)),bitshift(1,(bit_num-1)) )) );
        doInvert = xor(thisBit, bitshift(bitand(CRC,128),-7));
        CRC = bitshift(CRC,1);
        if(doInvert)
            CRC = bitxor(CRC,7);
        end
    end
end
end