#!/usr/bin/env python3
"""Compare exact BFS files and a bounded aligned range; no disk writes."""
import hashlib
import os
import stat
import struct
import subprocess
import sys

BLOCK=2048
GROUP_SHIFT=16
INODE_MAGIC=0x3bbe0ad9
FILES=(('state',7699963,41,((118,3694,1),)),
       ('trial.stderr',7699969,38,((118,3692,1),)),
       ('trial.stdout',7698697,4704,((118,3660,3),)),
       ('stable.backup',7699967,1062376,((118,65124,352),(119,62946,167))))

def require(value,why):
    if not value:raise RuntimeError(why)

def parse_inode(raw,number,size,expected,total_blocks):
    require(len(raw)==BLOCK,'short inode')
    require(struct.unpack_from('<I',raw,0)[0]==INODE_MAGIC,'inode magic')
    group,start,length=struct.unpack_from('<iHH',raw,4)
    require(group>=0 and length==1 and (group<<GROUP_SHIFT)+start==number,'inode address')
    require(struct.unpack_from('<q',raw,72+136)[0]==size,'inode size')
    extents=[]
    for i in range(12):
        g,s,n=struct.unpack_from('<iHH',raw,72+i*8)
        if not n:
            require(g==0 and s==0,'nonzero empty direct extent')
            continue
        require(g>=0 and (g<<GROUP_SHIFT)+s+n<=total_blocks,'extent outside volume')
        extents.append((g,s,n))
    require(tuple(extents)==expected,'unexpected allocation mapping')
    require(sum(n*BLOCK for _,_,n in extents)>=size,'short direct allocation')
    return extents

def digest(data):return hashlib.sha256(data).hexdigest()

def occurrences(data,needle):
    """Return every exact occurrence, including overlaps, without printing data."""
    require(bool(needle),'empty search pattern')
    offsets=[];position=0
    while True:
        position=data.find(needle,position)
        if position<0:return offsets
        offsets.append(position);position+=1

def read_exact(fd,size,offset):
    data=os.pread(fd,size,offset)
    require(len(data)==size,'short raw read')
    return data

def matching_runs(data,reference,block_size):
    """Summarize equal aligned blocks without exposing the bytes compared."""
    require(block_size>0 and len(data)==len(reference) and len(data)%block_size==0,
            'invalid aligned comparison')
    runs=[];start=None
    for index in range(len(data)//block_size):
        at=index*block_size
        equal=data[at:at+block_size]==reference[at:at+block_size]
        if equal and start is None:start=index
        if not equal and start is not None:runs.append((start,index));start=None
    if start is not None:runs.append((start,len(data)//block_size))
    return runs

def self_test():
    assert occurrences(b'ababa',b'aba')==[0,2]
    assert occurrences(b'ababa',b'xyz')==[]
    try:occurrences(b'ababa',b'')
    except RuntimeError:pass
    else:raise AssertionError('empty search pattern accepted')
    assert matching_runs(b'aabbccdd',b'aabbXXdd',2)==[(0,2),(3,4)]
    assert matching_runs(b'aa',b'bb',2)==[]
    for data,reference,block_size in ((b'a',b'a',0),(b'aa',b'a',1),(b'aaa',b'aaa',2)):
        try:matching_runs(data,reference,block_size)
        except RuntimeError:continue
        raise AssertionError('invalid aligned comparison accepted')
    for name,number,size,extents in FILES:
        raw=bytearray(BLOCK)
        struct.pack_into('<I',raw,0,INODE_MAGIC)
        struct.pack_into('<iHH',raw,4,number>>GROUP_SHIFT,number&65535,1)
        for i,extent in enumerate(extents):struct.pack_into('<iHH',raw,72+i*8,*extent)
        struct.pack_into('<q',raw,72+136,size)
        assert parse_inode(raw,number,size,extents,10000000)==list(extents)
        for position in (0,4,72,72+136):
            bad=bytearray(raw);bad[position]^=1
            try:parse_inode(bad,number,size,extents,10000000)
            except RuntimeError:continue
            raise AssertionError('corrupted inode accepted: '+name)
        try:parse_inode(raw,number,size,extents,1)
        except RuntimeError:continue
        raise AssertionError('out-of-range extent accepted: '+name)
    print('PASS four in-memory BFS inode mappings and corruption/range refusals; no disk access')

def output(command):return subprocess.check_output(command,text=True).strip()

def main():
    if sys.argv==[sys.argv[0],'--self-test']:
        self_test();return
    require(len(sys.argv)==1 and os.geteuid()==0,'run as root on the identified Linux host')
    require(output(['uname','-sm'])=='Linux riscv64','wrong platform')
    require(output(['lsblk','-dn','-o','SERIAL','/dev/sda']).replace(' ','')=='S14CNSAD300075H','wrong disk serial')
    require(output(['blockdev','--getsize64','/dev/sda'])=='120034123776','wrong disk size')
    require(output(['blockdev','--getsize64','/dev/sda1'])=='120033640448','wrong partition size')
    require(output(['blkid','-s','UUID','-o','value','/dev/sda1'])=='020147de08752551','wrong BFS UUID')
    require(open('/sys/class/block/sda1/start').read().strip()=='64','wrong partition start')
    device=os.stat('/dev/sda1');require(stat.S_ISBLK(device.st_mode),'not a block device')
    device_id=f'{os.major(device.st_rdev)}:{os.minor(device.st_rdev)}'
    with open('/proc/self/mountinfo') as mounts:
        require(all(line.split()[2]!=device_id for line in mounts),'partition is mounted')
    fd=os.open('/dev/sda1',os.O_RDONLY|os.O_CLOEXEC)
    try:
        sb=read_exact(fd,512,512)
        require(struct.unpack_from('<I',sb,32)[0]==0x42465331,'wrong BFS magic')
        require(struct.unpack_from('<I',sb,40)[0]==BLOCK and struct.unpack_from('<I',sb,76)[0]==GROUP_SHIFT,'wrong BFS geometry')
        total_blocks=struct.unpack_from('<q',sb,48)[0]
        require(0<total_blocks<=120033640448//BLOCK,'invalid filesystem size')
        files={}
        for name,number,size,expected in FILES:
            extents=parse_inode(read_exact(fd,BLOCK,number*BLOCK),number,size,expected,total_blocks)
            left=size;parts=[]
            for group,start,count in extents:
                amount=min(left,count*BLOCK)
                if amount:parts.append(read_exact(fd,amount,((group<<GROUP_SHIFT)+start)*BLOCK));left-=amount
            require(left==0,'incomplete data');files[name]=b''.join(parts)
            print(f'{name} inode={number} bytes={size} sha256={digest(files[name])} extents={extents}')
        require(digest(files['stable.backup'])=='041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403',
                'stable backup content changed')
        for name,offset in (('state',69632),('trial.stderr',65536),('trial.stdout',0)):
            matched=files[name]==files['stable.backup'][offset:offset+len(files[name])]
            print(f'{name} equals_backup_offset_{offset}={int(matched)}')
        # Examine whole allocated blocks, including bytes past logical EOF.
        # Do not emit their contents: they may contain unrelated old data.
        for name,_,_,extents in FILES[:3]:
            for group,start,count in extents:
                for index in range(count):
                    block=(group<<GROUP_SHIFT)+start+index
                    data=read_exact(fd,BLOCK,block*BLOCK)
                    offsets=occurrences(files['stable.backup'],data)
                    print(f'{name} allocation_block={block} whole_block_backup_offsets={offsets}')
        # The three observed file blocks and their backup offsets have the
        # same physical origin. Compare the intervening blocks too, without
        # displaying their data or assuming anything about current ownership.
        # These are raw reads only; a match does not prove prior allocation,
        # write ordering, cause, or future power-loss durability.
        origin=(118<<GROUP_SHIFT)+3660;count=len(files['stable.backup'])//BLOCK
        require(origin+count<=total_blocks and count*BLOCK<=len(files['stable.backup']),
                'aligned comparison outside verified volume or backup')
        raw=read_exact(fd,count*BLOCK,origin*BLOCK)
        matches=matching_runs(raw,files['stable.backup'][:count*BLOCK],BLOCK)
        print(f'aligned_backup_comparison physical_origin={origin} blocks={count} matching_half_open_block_ranges={matches}')
        print('READ ONLY: unmounted raw partition and verified inode extents; no filesystem or GPU changes')
    finally:os.close(fd)

try:main()
except (OSError,RuntimeError,subprocess.CalledProcessError,ValueError,IndexError) as exc:
    print('STOP: '+str(exc),file=sys.stderr);sys.exit(1)
