import os
import re

def extract_bytes(var_name, content):
    match = re.search(fr'static const unsigned char {var_name}\[\d+\] = {{(.*?)}};\n', content, re.DOTALL)
    if not match:
        return b''
    hex_str = match.group(1).replace('\n', '').replace(' ', '')
    bytes_list = []
    for x in hex_str.split(','):
        if x:
            bytes_list.append(int(x, 16))
    return bytes(bytes_list)

with open('office_templates.h', 'r') as f:
    header = f.read()

templates_dir = os.path.expanduser('~/.local/share/templates')
source_dir = os.path.join(templates_dir, '.source')
os.makedirs(source_dir, exist_ok=True)

docx = extract_bytes('s_docxTemplate', header)
xlsx = extract_bytes('s_xlsxTemplate', header)
pptx = extract_bytes('s_pptxTemplate', header)

with open(os.path.join(source_dir, 'Word Document.docx'), 'wb') as f:
    f.write(docx)
with open(os.path.join(source_dir, 'Excel Worksheet.xlsx'), 'wb') as f:
    f.write(xlsx)
with open(os.path.join(source_dir, 'PowerPoint Presentation.pptx'), 'wb') as f:
    f.write(pptx)
with open(os.path.join(source_dir, 'Text Document.txt'), 'wb') as f:
    f.write(b'')

def write_desktop(name, filename, icon):
    content = f'''[Desktop Entry]
Name={name}
Type=Link
URL=.source/{filename}
Icon={icon}
'''
    with open(os.path.join(templates_dir, f'{name}.desktop'), 'w') as f:
        f.write(content)

write_desktop('Word Document', 'Word Document.docx', 'application-vnd.openxmlformats-officedocument.wordprocessingml.document')
write_desktop('Excel Worksheet', 'Excel Worksheet.xlsx', 'application-vnd.openxmlformats-officedocument.spreadsheetml.sheet')
write_desktop('PowerPoint Presentation', 'PowerPoint Presentation.pptx', 'application-vnd.openxmlformats-officedocument.presentationml.presentation')
write_desktop('Text Document', 'Text Document.txt', 'text-plain')
