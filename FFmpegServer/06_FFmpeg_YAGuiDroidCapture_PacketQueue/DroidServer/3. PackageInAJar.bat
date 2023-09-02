SET DEX=%1

echo STEP 3 - PACKAGING in a JAR
echo Packaging %DEX%.dex in %DEX%.jar

jar cvf %DEX%.jar %DEX%.dex