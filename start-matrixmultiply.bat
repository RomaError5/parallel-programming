@echo off
setlocal enabledelayedexpansion

rem Использование: start-matrixmultiply.bat [block_cols] [block_rows] [matrix_size]
rem Пример: start-matrixmultiply.bat 32 16 800
set BLOCK_COLS=%1
if "%BLOCK_COLS%"=="" set BLOCK_COLS=16
set BLOCK_ROWS=%2
if "%BLOCK_ROWS%"=="" set BLOCK_ROWS=%BLOCK_COLS%
set MATRIX_SIZE=%3
if "%MATRIX_SIZE%"=="" set MATRIX_SIZE=200

if not exist ".venv" (
    echo Нет .venv, создаем...
    python -m venv .venv
    echo.
)

echo Активируем окружение...
call .venv\Scripts\activate.bat

echo.
echo Устанавливаем зависимости...
python -m pip install -r requirements.txt --cache-dir .pip

echo.
echo Генерируем матрицы...
python matrix_generate.py %MATRIX_SIZE%

echo.
echo Перемножаем матрицы с блоком %BLOCK_COLS%x%BLOCK_ROWS%......
bin\x64\Debug\MatrixMultiply.exe A.txt B.txt result.txt %BLOCK_COLS% %BLOCK_ROWS%

echo.
echo Проверяем результат...
python verify.py A.txt B.txt result.txt
if errorlevel 1 (
    echo Ошибка верификации!
) else (
    echo Верификация пройдена.
)

echo.
echo Очищаем временные файлы...
del A.txt B.txt result.txt 2>nul
echo Готово.
endlocal