import re
import sys
import os

def audit_patch(patch_path):
    invalid_files = []
    # Regex to match diff --git a/path b/path
    # Handling potential spaces by matching up to the next ' b/' or end of line
    pattern = re.compile(r'^diff --git a/(.*) b/(.*)$')
    
    with open(patch_path, 'r') as f:
        for line in f:
            match = pattern.match(line.strip())
            if match:
                path_a = match.group(1).strip()
                path_b = match.group(2).strip()
                
                # Check both paths (they should be same usually, but rename could differ)
                for path in [path_a, path_b]:
                    # Remove surrounding quotes if any (git might quote paths with spaces)
                    path = path.strip('"')
                    
                    is_valid = path.startswith('tests/') or path.endswith('Test.scala')
                    if not is_valid:
                        invalid_files.append(path)
                        
    return list(set(invalid_files))

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 audit_rtl_patch.py <patch_file>")
        sys.exit(1)
        
    patch_file = sys.argv[1]
    if not os.path.exists(patch_file):
        print(f"Error: File not found: {patch_file}")
        sys.exit(1)
        
    invalid = audit_patch(patch_file)
    if invalid:
        print("Found invalid files in patch:")
        for f in invalid:
            print(f"  {f}")
        sys.exit(1)
    else:
        print("Patch contains only allowed files (tests/ or *Test.scala).")
