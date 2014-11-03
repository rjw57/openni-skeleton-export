#!/usr/bin/env python
#
# An example script for extracting labelled images by associating points with
# their closest bone.
"""
Usage:
    labelbones.py (-h | --help)
    labelbones.py [--verbose] <logfile> <frame-prefix>

Options:
    -h, --help      Show a brief usage summary.
    -v, --verbose   Increase verbosity of output.
"""
import logging
import docopt
import numpy as np
import matplotlib.pyplot as plt
from PIL import Image
import tables

LOG = logging.getLogger()

def main():
    """Main entry point."""
    opts = docopt.docopt(__doc__)
    logging.basicConfig(
        level=logging.INFO if opts['--verbose'] else logging.WARN
    )

    LOG.info('Opening log file {0}'.format(opts['<logfile>']))
    log_root = tables.open_file(opts['<logfile>']).root

    for frame in log_root.frames:
        frame_idx = frame._v_attrs.idx
        if frame_idx % 30 == 0:
            LOG.info('Processing frame {0}...'.format(frame_idx))

        user = None
        for tracked_user in frame.users:
            try:
                if tracked_user.joints.shape[0] > 0:
                    user = tracked_user
            except AttributeError:
                # it's ok
                pass

        # If we have a user, detect labels
        if user is None:
            label_im = np.asanyarray(frame.label)
        else:
            label_im = bone_labels(frame, user)

        label_im = label_im / float(max(1.0, label_im.max()))
        label_color_im = (plt.cm.jet(label_im)[...,:3] * 255).astype(np.uint8)
        Image.fromarray(label_color_im).save(
            '{0}-{1:05d}.png'.format(opts['<frame-prefix>'], frame_idx))

def distances_to_line_segment(pts, line):
    """pts is a Nx3 array of 3d points.
    line = (p1, p2) where p1 and p2 are 3-vectors.
    """
    p1, p2 = line
    p1, p2 = np.asarray(p1), np.asarray(p2)

    # Let bone line be a + t * n

    # Compute n
    n = p2 - p1
    line_len = np.sqrt(np.sum(n**2))
    n /= line_len

    # Compute points using p1 and p2 as origin
    # Note, x = p - a
    x, y = np.copy(pts), np.copy(pts)
    for i in range(3):
        x[:,i] -= p1[i]
        y[:,i] -= p2[i]

    # Squared distances to p1 and p2
    d1 = np.sum(x**2, axis=1)
    d2 = np.sum(y**2, axis=1)

    # Compute t = (p - a) . n
    xdotn = np.copy(x)
    for i in range(3):
        xdotn[:,i] *= n[i]
    xdotn = np.sum(xdotn, axis=1)

    # Compute squared distance to line
    dl = np.zeros_like(xdotn)
    for i in range(3):
        dl += (x[:,i] - xdotn * n[i]) ** 2

    # Compute length along line
    norm_len = xdotn / line_len

    # Which distance should we use?
    d = np.where(norm_len < 0, d1, np.where(norm_len > 1, d2, dl))

    return np.sqrt(d)

def bone_labels(frame, user):
    # Get points for this user
    pts = frame.points[:]
    pt_labels = frame.point_labels[:]
    user_pts = pts[pt_labels == user._v_attrs.idx, :]

    joint_map = {}
    for joint in user.joints:
        joint_map[joint['id']] = (joint['x'], joint['y'], joint['z'])

    # Get bones
    bones = dict(
        neck = (1,2),
        left_forearm = (9,7), left_arm = (7,6),
        right_forearm = (13,15), right_arm = (12,13),
        left_chest = (6,17), right_chest = (12,21),
        left_thigh = (17,18), left_calf = (18,20),
        right_thigh = (21,22), right_calf = (22,24),
        left_collar = (2,6), right_collar = (6,12),
        # chest = (2,3)
    )

    bone_lines = {}
    for bone_name, bone_joints in bones.items():
        j1, j2 = bone_joints
        if j1 not in joint_map or j2 not in joint_map:
            continue
        j1_loc, j2_loc = tuple(joint_map[j] for j in (j1,j2))
        bone_lines[bone_name] = np.array((j1_loc, j2_loc))
    bone_names = sorted(bone_lines.keys())
    bone_dists = np.zeros((user_pts.shape[0], len(bone_names)))
    for i, n in enumerate(bone_names):
        bone_dists[:,i] = distances_to_line_segment(user_pts, bone_lines[n])

    closest_bone_indices = np.argmin(bone_dists, axis=1)
    label_image = np.zeros_like(frame.depth)
    label_image[frame.label == user._v_attrs.idx] = closest_bone_indices + 1

    return label_image

if __name__ == '__main__':
    main()

