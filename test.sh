find . -name "*.kicad_pcb" -print0 | while IFS= read -r -d '' file; do
            echo "Running DRC for $file"
            kicad-cli pcb drc "$file"
done
