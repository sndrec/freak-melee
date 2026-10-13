"""Assemble one inherited candidate without mutating accepted components."""
import hashlib,json,re
from pathlib import Path
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]
SOURCE='tests/native/peach_fighter_composition/candidate.fms'
original=(ROOT/SOURCE).read_text()
component=(HERE/'counter.fms').read_text()
assert original.count('article It_Kind_Peach_Toad states 2:')==1
original=re.sub(r'article It_Kind_Peach_Toad states 2:.*?(?=article It_Kind_Peach_Turnip)', '', original, flags=re.S)
declarations,policy=component.split('func pause_toad():',1)
candidate=original.replace('state int parasol_request_kind',declarations+'\nstate int parasol_request_kind',1)
candidate += '\nfunc pause_toad():'+policy
candidate=candidate.replace('# Float, Side-B, Toad and Parasol travel/animation remain pending.','# Float, Side-B and Parasol travel/animation remain pending.')
candidate=candidate.replace('# Partial Parasol/Toad tables below exist solely for accepted resource callbacks.','# Partial Parasol table exists solely for accepted resource callbacks.')
(HERE/'candidate.fms').write_text(candidate)
(HERE/'components.json').write_text(json.dumps({'source_inputs':{SOURCE:hashlib.sha256((ROOT/SOURCE).read_bytes()).hexdigest()},'composition_changes':['Accepted setup/Turnip/cleanup/Parasol producer unchanged; partial Toad declaration replaced.','Four complete counter states365..368 and Toad/Spore policies added.','Separate named pre/post hitlag bindings preserve stock Toad-only overwrite.','No registration, independent declaration or default activation.']},indent=2)+'\n')
