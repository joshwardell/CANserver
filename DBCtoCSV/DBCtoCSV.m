%Program to convert DBC file to xlsx
%Chris Allemang Jun 10 2020
%Requires comFramework, dbcParserForOctave.stg, dbcParser.m, hexID.m

clear
list = dbcParser('Model3CAN.dbc', 'CAN') %input DBC file name is defined
listLength = length (list.frameAry); %get number of messages
BUSData = cell(listLength,9); %initialize cell for storing data
jj=1; %start a counter
kk=1; %start a counter 
for i = 1:listLength %for every message
    messageName = list.frameAry(i).name; %get the message name
    messageID = hexID(list.frameAry(i).id); %get the messsage ID
    messageStartBit = 0; %reset message starting bit to 0
    for k = 1:length(list.frameAry(i).signalAry); %for every signal in a message
      messageStartBit(k)=list.frameAry(i).signalAry(k).startBit; %get the signal starting bit
      messageLength(k)=list.frameAry(i).signalAry(k).length; %get the signal bit length
      BUSData(jj,1) = messageName; %set column 1 to the message name
      BUSData(jj,2) = messageID; %set column 2 to the message  ID
      BUSData(jj,4) = list.frameAry(i).signalAry(k).name; %set column 4 to the signal name
      BUSData(jj,5) = messageStartBit(k); %set column 5 to the signal starting bit
      BUSData(jj,6) = messageLength(k); %set column 6 to the signal  length
      BUSData(jj,7) = list.frameAry(i).signalAry(k).factor; %set column 7 to the signal factor
      BUSData(jj,8) = list.frameAry(i).signalAry(k).offset; %set column 8 to the signal offst
      BUSData(jj,9) = list.frameAry(i).signalAry(k).unit; %set column 9 to the signal unit
      jj=jj+1; %count
    endfor
      [maxStartBit, maxStartBitLoc] = max(messageStartBit); %extract maximum start bit value and location
      messageLength = round((maxStartBit+messageLength(maxStartBitLoc))/8); %add maximum start bit and length of that signal and round to nearest byte to find message length
    for k = 1:length(list.frameAry(i).signalAry); %for every signal
      BUSData(kk,3)=messageLength; %set the message length 
      kk=kk+1; %count
    endfor
  end
%load packages needed for output file
pkg load io
pkg load windows
file_name='DBCtoCSV.xlsx'; %set output file name
xls=xlsopen(file_name,1); %open file
[xls, status] = oct2xls (BUSData,  xls); %save data
xls = xlsclose (xls); %close file 
