@echo off

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
python matrix_generate.py 200