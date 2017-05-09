#/usr/bin/python
import sys
import re

include_header = \
    '#ifdef _KERNEL\n#include <libcrunchk/types/typesheader.h>\n' \
    '#else\n#include <typesheader.h>\n' \
    '#endif\n\n'

# Given ['line1', 'line2', 'line3', 'line4'] and 'line3' will return
# ('line1\nline2', ['line3', 'line4'])
def split_up_to_string(lines, string):
    i = lines.index(string)
    res1 = '\n'.join(lines[:i])
    res2 = lines[i:]
    return (res1, res2)

# Make sure to put everything in .meta
def replace_sections(string):
    string = string.replace(
        'struct frame_allocsite_entry frame_vaddrs[] = {',
        'struct frame_allocsite_entry __attribute__((section (".meta"))) frame_vaddrs[] = {'
    )
    string = string.replace(
        'struct static_allocsite_entry statics[] = {',
        'struct static_allocsite_entry __attribute__((section (".meta"))) statics[] = {'
    )
    return string.replace('.data.', '.meta.')

# Any lines matching patterns in here gets removed
def remove_patterns(lines):
    subobj_pattern = re.compile("^.*subobj_names.*$")
    uniqtype_pattern = re.compile("^.*static alloc record for object __uniqtype_.*$")

    result = []
    liter = iter(lines[:])
    for l in liter :
        if subobj_pattern.match(l):
            continue
        if uniqtype_pattern.match(l):
            l = next(liter)
            l = next(liter)
            l = next(liter)
            continue
        result.append(l)
    return result



# args
with open(sys.argv[1], 'r') as f:
    lines = remove_patterns(f.read().split('\n'))

split_num = int(sys.argv[2])

# starting block (i.e. until '\n\n') into header
(header, lines) = split_up_to_string(lines, '')

# uniqtypes gets everything up until line containing end
(uniqtypes, lines) = split_up_to_string(
    lines,
    '/* Begin codeless (__uniqtype__<typename>) aliases. */'
)

# codeless gets next block
(codeless, lines) = split_up_to_string(
    lines,
    '/* Begin stack frame types. */'
)

# stacktypes get everything until alloc stuff starts
(stacktypes, lines) = split_up_to_string(lines, 'struct allocsite_entry')

# rest is alloc stuff
allocstuff = '\n'.join(lines)

# Debug
# with open('output', 'w') as f:
#     f.write('\n===========header==============================================\n')
#     f.write(header)
#     f.write('\n===========uniqtypes==============================================\n')
#     f.write(uniqtypes)
#     f.write('\n===========codeless==============================================\n')
#     f.write(codeless)
#     f.write('\n===========stacktypes==============================================\n')
#     f.write(stacktypes)
#     f.write('\n===========allocstuff==============================================\n')
#     f.write(allocstuff)
# sys.exit()


uniqtypes = uniqtypes.split('\n\n')
codeless = codeless.split('\n')
codeless = [x for x in codeless if x.startswith('extern struct uniqtype')]

regex = 'extern struct uniqtype [\w,$]+ __attribute__\(\(.*alias\("([\w,$]+)"\)\)\);.*'
codeless_map = {}
for v in codeless:
    try:
        k = re.match(regex, v).group(1)
    except:
        print('Regex failed!1')
        print(v)
        sys.exit()
    codeless_map[k] = v


print('Splitting into {} types*.c files'.format(split_num))
per_file = len(uniqtypes) / split_num + len(uniqtypes) % split_num
print('There are {} uniqtypes, so that\'s {} per file (apart from the last)'
      .format(len(uniqtypes), per_file))

for i in range(1, split_num + 1):
    output = ''
    print('Outputing number {}'.format(i))

    for uniqtype in uniqtypes[:per_file]:
        regex = '[.,\s]*__asm__\("\.size (.+), \d+"\)[.,\s]*'
        try:
            k = re.search(regex, uniqtype).group(1)
        except Exception as e:
            # This should just fail on comments, so ignore
            continue;

        # .get(k) because if not in codeless types don't worry about it
        output += uniqtype + '\n' + codeless_map.get(k, '') + '\n\n'
    uniqtypes = uniqtypes[per_file:]

    with open('types{}.c'.format(i), 'w') as f:
        f.write(include_header)
        f.write(replace_sections(output))


with open('typesheader.h', 'w') as f:
    f.write(replace_sections(header))

with open('typesstack.c', 'w') as f:
    f.write(include_header)
    f.write(replace_sections(stacktypes))
    f.write('\n\n')
    f.write(replace_sections(allocstuff))
