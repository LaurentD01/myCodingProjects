package social;

import jakarta.persistence.Entity;
import jakarta.persistence.Id;
import jakarta.persistence.FetchType;
import jakarta.persistence.ManyToOne;

import java.util.UUID;

@Entity
public class Post {

    @Id
    private String id;

    @ManyToOne(fetch = FetchType.EAGER)
    private Person autore;
    
    private String authorCode;
    private String text;
    private long timestamp;

    
    

    public Post() {
        // default constructor is needed by JPA
    }

    public Post(String authorCode, String text) { 
        this.authorCode = authorCode;
        this.text = text;
        this.id = UUID.randomUUID().toString().replace("-", "").substring(0,6);
        this.timestamp = System.currentTimeMillis();
    }

    public String getId() {
        return id;
    }

    public String getAuthorCode() {
        return authorCode;
    }

    public String getText() {
        return text;
    }

    public long getTimestamp() {
        return timestamp;
    }


}
