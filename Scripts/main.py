import os
import mangler
from dotenv import load_dotenv

FORTRAN_EXT = ".f90"
load_dotenv()

script_path = os.path.realpath(__file__)
project_path = os.path.join(os.path.dirname(script_path), os.pardir)
lib_path = os.path.join(project_path, "build", "libtestMangler.so")
test_dir = os.path.join(project_path, "BaseTests")
out_dir = os.path.join(project_path, "TestBin")

if os.path.exists(out_dir):
    # clear out old tests
    for filename in os.listdir(out_dir):
        old_test = os.path.join(out_dir, filename)
        if os.path.isfile(old_test):
            os.remove(old_test)
else:
    os.mkdir(out_dir)

flang = mangler.FlangUtil(os.environ["FLANG_EXEC"], lib_path)

for filename in os.listdir(test_dir):
    test_path = os.path.join(test_dir, filename)
    if os.path.isfile(test_path) and filename.endswith(FORTRAN_EXT):
        for index, output in enumerate(flang.iter(test_path), start=1):
            out_filename = "{}.{}.f90".format(filename[:-4], index)
            out_path = os.path.join(out_dir, out_filename)
            with open(out_path, "w") as file:
                file.write(output)

print("Finished generating tests")
