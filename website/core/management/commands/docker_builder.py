import re
import os
import time
import subprocess
import select
import tempfile
import datetime
import json
from django.core.management.base import BaseCommand
from core.models import SnakeVersion

def now():
    return datetime.datetime.utcnow().isoformat()

class Command(BaseCommand):
    help = "compile snake versions"

    def __init__(self):
        self.cleanup_re = re.compile(r"([^a-z0-9+_-]+)", re.IGNORECASE)

    def handle(self, *args, **options):
        while True:
            versions2build = SnakeVersion.objects.filter(compile_state='not_compiled')

            if versions2build:
                for s in versions2build:
                    self.build_version(s)
            else:
                print("Nothing to do, taking a nap...")
                time.sleep(5)

    def build_version(self, snake_version):
        return_code, output = self.run_build_script(snake_version)
        output.insert(0, {'i': 'compiling version ' + str(snake_version.version)})
        snake_version.build_log = json.dumps(output)
        snake_version.compile_state = 'successful' if return_code == 0 else 'failed'
        snake_version.save()

    def run_build_script(self, snake_version):
        BUILD_CWD = "../gameserver/docker4bots/"
        BUILD_SCRIPT = "./1_build_spn_bot.sh"

        code_file = self.write_code_to_temp_file(snake_version)
        print(f"{now()}: Code written to {code_file}")

        try:
            clean_name = self.cleanup_username(snake_version.user.username)
            cmd = [
                    BUILD_SCRIPT,
                    snake_version.programming_language.slug,
                    str(snake_version.id),
                    clean_name,
                    code_file,
                    snake_version.programming_language.file_extension]

            print(f"{now()}: Running: {cmd}")
            return_code, data = self.get_output_json(cmd, cwd=BUILD_CWD)
            print(f"{now()}: subprocess completed: {return_code}")
            return return_code, data

        finally:
            os.unlink(code_file)
            print(f"{now()}: {code_file} deleted.")

    @staticmethod
    def get_output_json(cmd, cwd):
        data = []
        with subprocess.Popen(cmd, cwd=cwd, stdout=subprocess.PIPE, stderr=subprocess.PIPE) as p:
            stdout = p.stdout.fileno()
            stderr = p.stderr.fileno()
            for fd, s in Command.read_fds([stdout, stderr], 1024):
                data.append({ 'e' if fd==stderr else 'o': s.decode() })
            p.wait()
            return p.returncode, data

    @staticmethod
    def read_fds(fds, maxread):
        while True:
            fds_in, _, _ = select.select(fds, [], [])
            for fd in fds_in:
                s = os.read(fd, maxread)
                if len(s) == 0:
                    fds.remove(fd)
                    continue
                yield fd, s
            if not len(fds):
                break

    @staticmethod
    def write_code_to_temp_file(snake_version):
        with tempfile.NamedTemporaryFile(mode='w', delete=False) as f:
            f.write(snake_version.code)
            return f.name

    def cleanup_username(self, username):
        return self.cleanup_re.sub("_", username)
