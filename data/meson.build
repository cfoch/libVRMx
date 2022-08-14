fs = import('fs') 

frag_shader = fs.read('frag.glsl')
vert_shader = fs.read('vert.glsl')

n_lines = 0
lines = frag_shader.split('\n')
frag_shader_data = '( \\\n'
foreach line: lines
  n_lines += 1
  if line.strip() == ''
    continue
  endif
  if n_lines != lines.length()
    line = '  "@0@\\n" \\\n'.format(line)
  else
    line = '  "@0@\\0" \\\n'.format(line)
  endif
  frag_shader_data += line
endforeach
frag_shader_data += ')'

n_lines = 0
lines = vert_shader.split('\n')
vert_shader_data = '( \\\n'
foreach line: lines
  n_lines += 1
  if line.strip() == ''
    continue
  endif
  if n_lines != lines.length()
    line = '  "@0@\\n" \\\n'.format(line)
  else
    line = '  "@0@\\0" \\\n'.format(line)
  endif
  vert_shader_data += line
endforeach
vert_shader_data += ')'
