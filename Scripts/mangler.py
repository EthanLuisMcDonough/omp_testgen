import os
import subprocess

class FlangUtil():
    def __init__(self, flang_path, lib_path):
        self.flang_path = flang_path
        self.lib_path = lib_path

    def gen_test(self, file, offset):
        plugin_args = [self.flang_path, "-fc1", "-fopenacc", 
                       "-fopenmp", "-load", self.lib_path, 
                       "-plugin", "test-mangle", file]
        plugin_process = subprocess.run(plugin_args, capture_output=True, 
                                        env=dict(os.environ, MANGLE_OFFSET=str(offset)))
        plugin_process.check_returncode()
        return plugin_process.stdout.decode()

    def iter(self, test_path):
        return iter(Mangler(self, test_path))

class Mangler():
    def __init__(self, flang, test_path):
        self.flang = flang
        self.path = test_path
        self.index = 1

    def __iter__(self):
        self.index = 1
        return self
    
    def __next__(self):
        try:
            output = self.flang.gen_test(self.path, self.index)
            self.index += 1
            return output
        except subprocess.CalledProcessError:
            raise StopIteration
