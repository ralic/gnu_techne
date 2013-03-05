in vec3 positions;
in vec3 normals;
in float sizes;

out cluster_attributes {
    vec3 center, normal;
    float size;
} cluster;

void main()
{   
    cluster.center = positions;
    cluster.normal = normals;
    cluster.size = sizes;
}
