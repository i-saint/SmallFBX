@echo off

cd Externals

IF NOT EXIST "7za.exe" (
    echo "downloading 7za.exe..."
    powershell.exe -NoProfile -InputFormat None -ExecutionPolicy Bypass -Command "[System.Net.ServicePointManager]::SecurityProtocol=[System.Net.SecurityProtocolType]::Tls12; wget https://github.com/i-saint/SmallFBX/releases/download/data/7za.exe -OutFile 7za.exe"
)

echo "downloading external libararies..."
powershell.exe -NoProfile -InputFormat None -ExecutionPolicy Bypass -Command "[System.Net.ServicePointManager]::SecurityProtocol=[System.Net.SecurityProtocolType]::Tls12; wget https://github.com/i-saint/SmallFBX/releases/download/data/Externals.7z -OutFile Externals.7z"
7za.exe x -aos Externals.7z

cd ..
