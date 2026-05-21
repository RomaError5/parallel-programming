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