build:
    source $HOME/esp/esp-idf/export.sh && idf.py build
    cp ./build/compile_commands.json ./compile_commands.json

flash:
    source $HOME/esp/esp-idf/export.sh && idf.py flash

clean:
    rm -fRd build/
    rm -fRd compile_commands.json
    rm -fRd .cache/

