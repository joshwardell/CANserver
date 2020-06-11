%convertStringValue to hex
%Chris Allemang Jun 10 2020
%To be used with DBCtoCSV.m

function [hexIDString] = hexID(ID) %function decleration 
  if numel(num2str(dec2hex(str2num(ID)))) < 2 %if the hex is only one digit
    hexIDString = strcat('0x00',num2str(dec2hex(str2num(ID)))); %add 0x00 to start
elseif numel(num2str(dec2hex(str2num(ID)))) < 3 %if the hex is only two digits
    hexIDString = strcat('0x0',num2str(dec2hex(str2num(ID)))); %add 0x0
else %otherwise
    hexIDString = strcat('0x',num2str(dec2hex(str2num(ID)))); %add 0x
end
endfunction
