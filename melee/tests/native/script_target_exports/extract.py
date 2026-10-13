"""Reuse complete production source guards and native Fighter lookup."""
import hashlib,importlib.util
from pathlib import Path
def extract(root,out):
 path=Path(root)/'tests/native/script_target_queries/extract.py'
 s=importlib.util.spec_from_file_location('target_base',path);m=importlib.util.module_from_spec(s);s.loader.exec_module(m)
 hashes=m.extract(root,out);hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest();return hashes
