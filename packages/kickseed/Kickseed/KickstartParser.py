import string

class KickstartNotImplemented(Exception): pass

class KickstartParser:
    def __init__(self):
        # TODO: data should be a class itself
        self.data = {}

    def read(self, fd):
        sections = {}
        for section in 'main', 'packages', 'pre', 'post':
            sections[section] = []

        section = 'main'
        for line in fd:
            line = string.strip(line)

            if line == '':
                continue
            elif line.startswith('%packages'):
                section = 'packages'
            elif line.startswith('%pre'):
                section = 'pre'
            elif line.startswith('%post'):
                section = 'post'

            sections[section].append(line)

        for line in sections['main']:
            if line == '':
                continue
            elif line.startswith('#'):
                continue
            else:
                tokens = string.split(line)
                self.data[tokens[0]] = tokens[1:]

        if sections['packages']:
            # TODO: options to %packages
            # TODO: groups

            self.data['individualPackageList'] = []

            for pkg in sections['packages'][1:]:
                if pkg.startswith('@'):
                    # TODO: groups
                    raise KickstartNotImplemented, "Package groups not implemented"
                else:
                    self.data['individualPackageList'].append(pkg)

        if sections['pre']:
            tokens = string.split(sections['pre'][0])
            self.data['preLine'] = tokens[1:]
            self.data['preList'] = sections['pre'][1:]

        if sections['post']:
            tokens = string.split(sections['post'][0])
            self.data['postLine'] = tokens[1:]
            self.data['postList'] = sections['post'][1:]

        # TODO: I don't like this ...
        return self.data
