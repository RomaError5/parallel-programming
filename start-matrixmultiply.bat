@echo off
setlocal enabledelayedexpansion

rem Использование: start-matrixmultiply.bat [число_процессов] [размер_матрицы]
set BLOCK_SIZE=%1
if "%BLOCK_SIZE%"=="" set BLOCK_SIZE=8
set MATRIX_SIZE=%2
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
echo Перемножаем матрицы...
bin\x64\Debug\MatrixMultiply.exe A.txt B.txt result.txt %BLOCK_SIZE%

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