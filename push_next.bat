@echo off
cd /d "%~dp0"

set "TARGET="
for /f "delims=" %%c in ('git rev-list --reverse --ancestry-path origin/master..master') do (
    if not defined TARGET set "TARGET=%%c"
)

if not defined TARGET (
    echo push するコミットがありません
    pause
    exit /b 1
)

echo pushing %TARGET%
git push origin %TARGET%:master
pause