#/usr/bin/python
import sys
import re

# Given ['line1', 'line2', 'line3', 'line4'] and 'line3' will return
# ('line1\nline2', ['line3', 'line4'])
def split_up_to_string(lines, string):
    i = lines.index(string)
    res1 = '\n'.join(lines[:i])
    res2 = lines[i:]
    return (res1, res2)


# args
with open(sys.argv[1], 'r') as f:
    lines = f.read().split('\n')

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
# Debug
# print(codeless_map.keys()[0])
# print(codeless_map[codeless_map.keys()[0]])
# print(codeless_map['__uniqtype_c95a47f2__usr_obj_usr_src_sys_CRUNCHED___machine_pmap_h_307'])


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
            # print(e)
            # print('Regex failed!2')
            # print(uniqtype)
            continue;

        # .get(k) because if not in codeless types don't worry about it
        output += uniqtype + '\n' + codeless_map.get(k, '') + '\n\n'
    uniqtypes = uniqtypes[per_file:]

    with open('types{}.c'.format(i), 'w') as f:
        f.write('#include <libcrunchk/types/typesheader.h>\n\n')
        f.write(output)


with open('typesheader.h', 'w') as f:
    f.write(header)

with open('typesstack.c', 'w') as f:
    f.write('#include <libcrunchk/types/typesheader.h>\n\n')
    f.write(stacktypes)
    f.write('\n\n')
    f.write(allocstuff)
