@echo off
setlocal
node "%~dp0cli\render-card.js" %*
exit /b %errorlevel%
