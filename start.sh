NUM_THREADS=${1:-1}
MATRIX_SIZE=${2:-100}

if [ ! -d ".venv" ]; then
    echo "Нет .venv, создаем..."
    python -m venv .venv
    echo -e "\n"
fi

echo -e "Активируем окружение..."
source .venv/bin/activate

echo -e "\nУстанавливаем зависимости..."
python -m pip install -r requirements.txt

echo -e "\nГенерируем матрицы..."
python matrix_generate.py $MATRIX_SIZE

#Компилируем код ручками через Developer Command Prompt
#cl /O2 /openmp /EHsc matrix_multiply.cpp /Fe:matrix_multiply.exe

echo -e "\nПеремножаем матрицы..."
./matrix_multiply.exe A.txt B.txt result.txt $NUM_THREADS

echo -e "\nПроверяем результат..."
python verify.py A.txt B.txt result.txt

echo -e "\nОчищаем результаты..."
rm A.txt B.txt result.txt